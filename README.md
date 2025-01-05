# SandSaga
Sandfalling simulator in C

# Index
- [Pre-build requirements](#pre-build-requirements)
- [Build and Run](#build-and-run)
- [System requirements (minimum)](#system-requirements-minimum)
- [Some resources license or apreciation](#some-resources-license-or-apreciation)


## Pre-build requirements
1. Install dependencies: `cat requirements.txt |xargs sudo apt install`
2. Get compilers for arm64 and amd64. **If** you're only gonna build only for your **native** system, **at least** you need to setup the **mock compilers**.
```
# ==== Build ARM (from x86-64 native) ====
# Get compilers for aarch64
sudo dpkg --add-architecture arm64 && sudo apt update
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
# IMPORTANT! Link mock compiler to native x86-64 compiler
sudo ln -s /usr/bin/gcc /usr/bin/x86-64-linux-gnu-gcc
sudo ln -s /usr/bin/g++ /usr/bin/x86-64-linux-gnu-g++
```
```
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

### (Optional) Build for Windows from linux (*Only x86-64*)
1. Make sure you installed mingw-w64 in the previous step
2. Paste the following snippet to download the required libraries for mingw
```
mkdir mingw_libs
```
SDL2 libs *(prebuilt)*:
```
# SDL
wget -P mingw_libs https://github.com/libsdl-org/SDL/releases/download/release-2.26.5/SDL2-devel-2.26.5-mingw.tar.gz
# SDL_image
wget -P mingw_libs https://github.com/libsdl-org/SDL_image/releases/download/release-2.6.3/SDL2_image-devel-2.6.3-mingw.tar.gz
```
Box2D libs *(sadly, not prebuilt)*:
```
# Clone the Box2D repository
git clone -b 'v2.4.1' --single-branch --depth 1 https://github.com/erincatto/box2d mingw_libs/box2d
mkdir mingw_libs/box2d/build_mingw
cd mingw_libs/box2d/build_mingw
cmake -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ -DBUILD_SHARED_LIBS=ON -DBOX2D_BUILD_UNIT_TESTS=OFF -DBOX2D_BUILD_DOCS=OFF -DCMAKE_INSTALL_PREFIX=$(pwd)/../install_mingw ..
make -j$(nproc)
make install
cd ../../..
```
3. Extract the libraries in mingw_libs
```
for a in mingw_libs/*.tar.gz; do tar zxvf $a -C mingw_libs; done
rm mingw_libs/*.tar.gz
```
4. NOTE: In order to play it, you need to run CPack to gather all the DLLs and other resources.

# Build and Run
- Use vscode extension for CMake, select a preset and launch it with **CTRL+F5**
- For windows, you can test with `wine ./game_sdl2`.

# System requirements (minimum)
The following table repesents the worst hardware where the game has been tested to work at ~60 FPS stable.
|               |            **Windows (x86-64)**           |             **Linux (x86-64)**            |            **Linux (aarch64)**            |
|--------------:|:------------------------------------------|:------------------------------------------|:------------------------------------------|
| **Processor** | Intel Core i3 4130                        | Intel Core i3 4130 / AMD Ryzen 3 1200     | Rockchip RK3588                           |
|    **Memory** | 4 GB RAM                                  | 4 GB RAM                                  | 4 GB RAM                                  |
|  **Graphics** | Intel HD Graphics 530 / AMD Radeon R5 240 | Intel HD Graphics 530 / AMD Radeon R5 240 | Intel HD Graphics 530 / AMD Radeon R5 240 |
|   **Storage** | 28 MB                                     | 144 KB                                    | 144 KB                                    |
|    **System** | Windows NT, DirectX11                     | GLIBC_2.34, GLIBCXX_3.4.29                | GLIBC_2.34, GLIBCXX_3.4.29                |
> (*) System requirements might be not minimum, but exact version required (in some cases).

# Some resources license or apreciation
### FNTCOL16
- These fonts come from http://ftp.lanet.lv/ftp/mirror/x2ftp/msdos/programming/misc/fntcol16.zip
- The package is (c) by Joseph Gil
- The individual fonts are public domain
### Third-party algorithms implemented
- https://github.com/Lecanyu/RDPSimplify
- https://github.com/mapbox/earcut.hpp/
