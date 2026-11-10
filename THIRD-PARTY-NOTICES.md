# Third-party notices

schwaystation is MIT-licensed ([LICENSE](LICENSE)). A few files come from other projects under permissive
licences. Each section below names those files, where they came from, what changed, and the licence text the
upstream requires to travel with source and binaries. [PROVENANCE.md](PROVENANCE.md) says when a section is
needed.

## libretro.h

- **Applies to:** `libretro/libretro.h`
- **Upstream:** https://github.com/libretro/libretro-common, file `include/libretro.h` at commit
  `a3194c18ba65ef89734e1898a2f61403c255d368`. The file's own header licenses it MIT.
- **SHA-256:** `036ddffc995ad116c3081ca0a7515d09c2a116f3483e4f5012fe0c29a885da76`
- **Changes:** none. The file is vendored unmodified.
- **Licence:** MIT

```
Copyright (C) 2010-2024 The RetroArch team

Permission is hereby granted, free of charge,
to any person obtaining a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```

## SingleStepTests/r3000 vectors

- **Applies to:** `tests/vectors/r3000/sst-sample.bin`
- **Upstream:** https://github.com/SingleStepTests/r3000, the 55 files `v1/*.json.bin` at commit
  `51e227447d0b93882a8f13c87c6cee72819efdd3`. The repository's `LICENSE` is MIT.
- **SHA-256:** `d6ef9a9ce978c487d2065c60c7312959a8bb6ddbd67b19c8465d1e28557d5ffc`
- **Changes:** the file holds the first 100 records of each upstream file, byte for byte, in one container
  (`tools/sst_slice.py`); each slice's leading case count is changed from 1,000 to 100.
- **Licence:** MIT

```
MIT License

Copyright (c) 2025 SingleStepTests

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## ps1-tests GTE cases

- **Applies to:** `tests/vectors/gte/ps1tests-1150.bin` and `tests/fixtures/src/gte_cases.inc`
- **Upstream:** https://github.com/JaCzekanski/ps1-tests, the file `gte/test-all/tests.c` at commit
  `f727802fead11f1daa7549285548392ef87749cb`. The repository's `LICENSE` is MIT.
- **SHA-256:** `e8a0e61a57a6582ac53d39b941eeaa83b87498444a3ada915d9de73d313dd9c2` (the `.bin`; the include's digest is in `tests/fixtures/manifest.txt`)
- **Changes:** transcoded by `tools/gte2bin.py`; values unchanged. The `.bin` holds all 1,150 cases, the
  include 26 of them, copied verbatim; case names are dropped and case N is the upstream "test N".
- **Provenance:** hardware-captured per upstream, not verified here.
- **Licence:** MIT

```
MIT License

Copyright (c) 2020 Jakub Czekański

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
