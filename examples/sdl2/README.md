# SDL2 example host

This optional example shows how a host can load a BIOS and CUE/ISO/M3U image, pass sectors through the public disc
callbacks, and present video, audio and digital-pad input. The window, audio device, keyboard and image-provider
code stay in this example; none is part of the library, runner or libretro core.

Requirements: CMake, a C17 compiler, SDL2 development files discoverable as `sdl2` through `pkg-config`, and a
PlayStation BIOS and supported image available on your machine.

From the repository root, install the library component and build the example against that installed package:

```sh
cmake --preset ci
cmake --build --preset ci --target schwaystation
cmake --install build/ci --prefix build/ci --component library
cmake -S examples/sdl2 -B build/example-sdl2 -DCMAKE_PREFIX_PATH="$PWD/build/ci" \
  -Dschwaystation_DIR="$PWD/build/ci/lib/cmake/schwaystation"
cmake --build build/example-sdl2
```

Run it with a disc image and BIOS path:

```sh
build/example-sdl2/schwaystation-sdl2 /path/to/disc.cue /path/to/bios.bin
```

Arrow keys move; Enter is Start; Backspace is Select; Z/X/S/A are Cross/Circle/Triangle/Square. F1 opens or closes
the lid. With an M3U loaded and the lid open, F2 selects the next image. This example does not save states.
