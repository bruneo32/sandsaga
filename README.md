# SandSaga
Sandfalling simulator in C

## Pre-build
1. Install dependencies: `cat requirements.txt |xargs sudo apt install`
2. Bake assets before building (*or when they change*): `./bake_assets.sh`

### (Optional) Build for Windows from linux
1. Make sure you installed mingw-w64 in the previous step
2. Paste the following snippet to download the required libraries for mingw
```
mkdir mingw_libs
wget -P mingw_libs https://github.com/libsdl-org/SDL_image/releases/download/release-2.8.2/SDL2_image-devel-2.8.2-mingw.tar.gz
wget -P mingw_libs https://github.com/libsdl-org/SDL/releases/download/release-2.30.9/SDL2-devel-2.30.9-mingw.tar.gz
```
3. Extract the libraries in mingw_libs
```
for a in mingw_libs/*.tar.gz; do tar zxvf $a -C mingw_libs; done
rm mingw_libs/*.tar.gz
```
4. NOTE: Don't forget to ship the DLLs with the executable: (After building the first time)
```
cp mingw_libs/SDL2*/x86_64-w64-mingw32/bin/*.dll build/win64-mingw32/debug/
cp mingw_libs/SDL2*/x86_64-w64-mingw32/bin/*.dll build/win64-mingw32/release/
```

# Build and Run
- Use vscode extension for CMake, select a preset and launch it with **CTRL+F5**

# Some resources license or apreciation
### FNTCOL16
- These fonts come from http://ftp.lanet.lv/ftp/mirror/x2ftp/msdos/programming/misc/fntcol16.zip
- The package is (c) by Joseph Gil
- The individual fonts are public domain
