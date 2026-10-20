# 32-bit big-endian PowerPC Linux, the be32 lane: big-endian, ILP32 and
# unsigned plain char in one target. Tests run under qemu-user with the cross
# sysroot as its library prefix (Ubuntu packages gcc-powerpc-linux-gnu,
# g++-powerpc-linux-gnu and qemu-user).
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR powerpc)
set(CMAKE_C_COMPILER powerpc-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER powerpc-linux-gnu-g++)
set(CMAKE_FIND_ROOT_PATH /usr/powerpc-linux-gnu)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(CMAKE_CROSSCOMPILING_EMULATOR qemu-ppc;-L;/usr/powerpc-linux-gnu)
