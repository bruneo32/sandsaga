# Sandsaga
Sandfalling simulator in C

# Index
- [Pre-build requirements](#pre-build-requirements)
- [Build and Run](#build-and-run)
- [System requirements (minimum)](#system-requirements-minimum)
- [Some resources license and appreciation](#some-resources-license-and-appreciation)
- [Trademark notice](#trademark-notice)


## Pre-build requirements
1. Install dependencies: `cat requirements.txt |xargs sudo apt install`
2. Get compilers for arm64 and amd64. **If** you're only gonna build only for your **native** system, **at least** you need to setup the **mock compilers**.
```sh
# ==== Build ARM (from x86-64 native) ====
# Get compilers for aarch64
sudo dpkg --add-architecture arm64 && sudo apt update
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
# IMPORTANT! Link mock compiler to native x86-64 compiler
sudo ln -s /usr/bin/gcc /usr/bin/x86-64-linux-gnu-gcc
sudo ln -s /usr/bin/g++ /usr/bin/x86-64-linux-gnu-g++
```
```sh
# ==== Build x86-64 (from ARM native) ====
# Get compilers for x86-64
sudo dpkg --add-architecture amd64 && sudo apt update
sudo apt install gcc-x86-64-linux-gnu g++-x86-64-linux-gnu
# IMPORTANT! Link mock compiler to native aarch64 compiler
sudo ln -s /usr/bin/gcc /usr/bin/aarch64-linux-gnu-gcc
sudo ln -s /usr/bin/g++ /usr/bin/aarch64-linux-gnu-g++
```
> Note: The previous commands are for debian, you shall adecuate it to your distro requirements.

2. Bake assets before building (*or when they change*): `./bake_assets.sh`

3. Run `./make_libs.sh all` this will automatically download and build all the required static libs like SDL, box2d, etc. \
**This will take a while, go for a coffee.**

# Build and Run
1. You can use vscode extension for CMake for configure/build/pack. *(This is what I use because it's comfortable).*
2. If you want to build from console, select a preset from `CMakePresets.json` as follows:
```sh
cmake --preset linux-release-x86_64      # Select preset
cmake --build build/linux-x86_64/release # Build it
cpack --preset linux-release-x86_64      # Pack it for distribution
```

> NOTE: For windows, you can test with `find dist -name sandsaga.exe |xargs wine`.

# System requirements (minimum)
The following table repesents the worst hardware where the game has been tested to work at ~60 FPS stable.
|               |            **Windows (x86-64)**           |             **Linux (x86-64)**            |            **Linux (aarch64)**            |
|--------------:|:-----------------------------------------:|:-----------------------------------------:|:-----------------------------------------:|
| **Processor** | Intel Core i3 4130 / AMD Ryzen 3 1200     | Intel Core i3 4130 / AMD Ryzen 3 1200     | Qualcomm Snapdragon 625 / Rockchip RK3588 |
|    **Memory** | 0.99 GB RAM                               | 0.99 GB RAM                               | 0.99 GB RAM                               |
|  **Graphics** | Intel HD Graphics 530 / AMD Radeon R5 240 | Intel HD Graphics 530 / AMD Radeon R5 240 | Intel HD Graphics 530 / AMD Radeon R5 240 |
|   **Storage** | 2 GB                                      | 2 GB                                      | 2 GB                                      |
|    **System** | Windows NT, DirectX11                     | GLIBC_2.29, GLIBCXX_3.4.29 *              | GLIBC_2.29, GLIBCXX_3.4.29 *              |
> (*) GLIBC, and GLIBCXX shall be ABI compatible with this version.

# Some resources license and appreciation
### FNTCOL16
- These fonts come from http://ftp.lanet.lv/ftp/mirror/x2ftp/msdos/programming/misc/fntcol16.zip
- The package is (c) by Joseph Gil
- The individual fonts are public domain
### Emanuele Feronato
- https://emanueleferonato.com/2013/03/01/using-marching-squares-algorithm-to-trace-the-contour-of-an-image/
### Third-party algorithms implemented
- https://github.com/Lecanyu/RDPSimplify
- https://github.com/mapbox/earcut.hpp/

# Trademark notice
The name "Sandsaga" and the associated logo are trademarks of Bruno Castro García and are intended to identify the original, authentic version of the project.\
See [TRADEMARK.md](TRADEMARK.md) for more information.
