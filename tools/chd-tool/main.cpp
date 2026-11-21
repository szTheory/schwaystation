#include "chd_shim.h"
#include "chd.h"
#include "chdcodec.h"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace {

struct toc_deleter {
  void operator()(chd_shim_toc_t *toc) const { chd_shim_toc_free(toc); }
};

struct compressor_deleter {
  void operator()(chd_file_compressor_t *compressor) const {
    chd_shim_compressor_free(compressor);
  }
};

struct output_cleanup {
  const std::string &path;
  const bool &created;
  const bool &keep;

  ~output_cleanup() {
    if (created && !keep) {
      std::remove(path.c_str());
    }
  }
};

#if defined(_WIN32)
using file_status = struct _stat64;
int get_file_status(const char *path, file_status *result) { return _stat64(path, result); }
bool is_regular_file(const file_status &result) {
  return (result.st_mode & _S_IFMT) == _S_IFREG;
}
#else
using file_status = struct stat;
int get_file_status(const char *path, file_status *result) { return stat(path, result); }
bool is_regular_file(const file_status &result) { return S_ISREG(result.st_mode); }
#endif

bool path_exists_or_cannot_check(const char *path) {
  file_status result{};
  if (get_file_status(path, &result) == 0) {
    return true;
  }
  return errno != ENOENT;
}

bool positive_file_size(const char *path, uint64_t *size) {
  file_status result{};
  if (get_file_status(path, &result) != 0 || !is_regular_file(result) || result.st_size <= 0) {
    return false;
  }
  *size = static_cast<uint64_t>(result.st_size);
  return true;
}

bool set_single_worker() {
#if defined(_WIN32)
  return _putenv_s("OSDWORKQUEUEMAXTHREADS", "1") == 0;
#else
  return setenv("OSDWORKQUEUEMAXTHREADS", "1", 1) == 0;
#endif
}

int fail(const char *operation, int32_t error) {
  std::cerr << "schwaystation-chd-tool: " << operation << " failed (CHD error "
            << error << ")\n";
  return 1;
}

} // namespace

int main(int argc, char **argv) {
  if (argc == 2 && std::string(argv[1]) == "--print-settings") {
    std::cout << "format=schwaystation-chd-tool-v1\n"
              << "codecs=cdlz,cdzl,cdfl,0\n"
              << "hunk_bytes=19584\n"
              << "unit_bytes=2448\n"
              << "workers=1\n";
    return 0;
  }
  if (argc != 3) {
    std::cerr << "usage: schwaystation-chd-tool <source.cue> <output.chd>\n";
    return 2;
  }
  if (!set_single_worker()) {
    std::cerr << "schwaystation-chd-tool: could not pin the worker count\n";
    return 1;
  }

  const std::string cue_path(argv[1]);
  const std::string output_path(argv[2]);
  file_status input_status{};
  if (get_file_status(cue_path.c_str(), &input_status) != 0 || !is_regular_file(input_status)) {
    std::cerr << "schwaystation-chd-tool: input CUE is not a regular file\n";
    return 2;
  }
  if (path_exists_or_cannot_check(output_path.c_str())) {
    std::cerr << "schwaystation-chd-tool: output path already exists or cannot be checked\n";
    return 2;
  }

  bool output_created = false;
  bool keep_output = false;
  [[maybe_unused]] const output_cleanup cleanup{output_path, output_created, keep_output};
  std::unique_ptr<chd_shim_toc_t, toc_deleter> toc(chd_shim_toc_alloc());
  if (!toc) {
    std::cerr << "schwaystation-chd-tool: cannot allocate the CD table of contents\n";
    return 1;
  }
  const int32_t toc_error = chd_shim_toc_parse(toc.get(), cue_path.c_str());
  if (toc_error != 0) {
    return fail("CUE parse", toc_error);
  }
  chd_shim_toc_pad_tracks(toc.get());
  const uint64_t logical_bytes = chd_shim_toc_logical_bytes(toc.get());
  if (logical_bytes == 0) {
    std::cerr << "schwaystation-chd-tool: CUE has no usable CD frames\n";
    return 1;
  }

  std::unique_ptr<chd_file_compressor_t, compressor_deleter> compressor(
      chd_shim_cd_compressor_alloc(toc.get()));
  if (!compressor) {
    std::cerr << "schwaystation-chd-tool: cannot allocate the CD compressor\n";
    return 1;
  }

  constexpr uint32_t frame_bytes = 2448;
  constexpr uint32_t hunk_bytes = 8 * frame_bytes;
  const uint32_t codecs[4] = {
      CHD_CODEC_CD_LZMA, CHD_CODEC_CD_ZLIB, CHD_CODEC_CD_FLAC, CHD_CODEC_NONE};
  output_created = true;
  int32_t error = chd_shim_compressor_create_file(
      compressor.get(), output_path.c_str(), logical_bytes, hunk_bytes, frame_bytes, codecs);
  if (error != 0) {
    return fail("CHD create", error);
  }

  auto *chd = reinterpret_cast<chd_file_t *>(compressor.get());
  error = chd_shim_cd_write_metadata(chd, toc.get());
  if (error != 0) {
    return fail("CD metadata write", error);
  }

  chd_shim_compressor_begin(compressor.get());
  while (true) {
    double progress = 0.0;
    double ratio = 0.0;
    error = chd_shim_compressor_continue(compressor.get(), &progress, &ratio);
    if (error == 0) {
      break;
    }
    if (error != static_cast<int32_t>(chd_file::error::WALKING_PARENT)
        && error != static_cast<int32_t>(chd_file::error::COMPRESSING)) {
      return fail("CHD compression", error);
    }
  }

  compressor.reset();
  uint64_t output_size = 0;
  if (!positive_file_size(output_path.c_str(), &output_size)) {
    std::cerr << "schwaystation-chd-tool: output file is missing or empty\n";
    return 1;
  }
  keep_output = true;
  std::cout << "created " << output_size << " bytes\n";
  return 0;
}
