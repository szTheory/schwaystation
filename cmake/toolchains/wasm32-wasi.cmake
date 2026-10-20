# WebAssembly with WASI preview 1, the wasm32 lane: the distribution's
# clang-21 and lld-21 with wasi-libc and libclang-rt-21-dev-wasm32. The triple
# is wasm32-wasi because that compiler-rt layout breaks wasm32-wasip1 links.
# lime1 is a frozen feature set (generic drifts between releases); no SIMD,
# threads or wasip2. --stack-first puts the stack below static data, so an
# overflow traps instead of corrupting data and changing a hash.
set(CMAKE_SYSTEM_NAME WASI)
set(CMAKE_SYSTEM_PROCESSOR wasm32)
set(CMAKE_C_COMPILER clang-21)
set(CMAKE_CXX_COMPILER clang++-21)
set(CMAKE_C_COMPILER_TARGET wasm32-wasi)
set(CMAKE_CXX_COMPILER_TARGET wasm32-wasi)
set(CMAKE_C_FLAGS_INIT "-mcpu=lime1 -D_WASI_EMULATED_SIGNAL")
set(CMAKE_CXX_FLAGS_INIT "-mcpu=lime1 -D_WASI_EMULATED_SIGNAL")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-Wl,--stack-first")
set(CMAKE_C_STANDARD_LIBRARIES -lwasi-emulated-signal)
# There is no C++ standard library for this target; C++ only compiles the
# public header, so compiler checks build a static library instead of a
# program.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
