#!/bin/bash

# Save working directory for later
_CWD=$(pwd)

# Show help
if [[ -z "$1" ]]; then
	echo "Usage: $(basename $0) [all|clean|amd64|arm64|mingw]"
	exit 1
fi

# Initialize TARGETS array with provided arguments
TARGETS=("$@")

# Select targets
if [[ " ${TARGETS[@]} " =~ " all " ]]; then
	# Append all targets
	TARGETS+=( "amd64" "arm64" "mingw" )
fi
echo "Building for targets: ${TARGETS[*]}"

# Define global flags
AMD64_FLAGS="-march=x86-64 -m64 -mfma -mavx -mavx2 -mabm -msse4.2"
ARM64_FLAGS="-march=armv8-a -mfix-cortex-a53-835769"

COMMON_FLAGS="-mtune=generic -funroll-loops -ffast-math \
	-fopenmp -ftree-vectorize -falign-functions=32 \
	-O2 -s -DNDEBUG -fno-stack-protector -fomit-frame-pointer"
COMMON_FLAGS_WIN="$COMMON_FLAGS -mno-ms-bitfields"

FLAGS_AMD64=(
	-DCMAKE_SYSTEM_NAME=Linux
	-DCMAKE_SYSTEM_PROCESSOR=x86_64
	-DCMAKE_C_COMPILER=x86-64-linux-gnu-gcc
	-DCMAKE_CXX_COMPILER=x86-64-linux-gnu-g++
	-DCMAKE_BUILD_TYPE=Release
	-DCMAKE_C_FLAGS_RELEASE="${COMMON_FLAGS} ${AMD64_FLAGS}"
	-DCMAKE_CXX_FLAGS_RELEASE="${COMMON_FLAGS} ${AMD64_FLAGS}"
)

FLAGS_ARM64=(
	-DCMAKE_SYSTEM_NAME=Linux
	-DCMAKE_SYSTEM_PROCESSOR=aarch64
	-DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc
	-DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++
	-DCMAKE_BUILD_TYPE=Release
	-DCMAKE_C_FLAGS_RELEASE="${COMMON_FLAGS} ${ARM64_FLAGS}"
	-DCMAKE_CXX_FLAGS_RELEASE="${COMMON_FLAGS} ${ARM64_FLAGS}"
)

FLAGS_MINGW=(
	-DCMAKE_SYSTEM_NAME=Windows
	-DCMAKE_SYSTEM_PROCESSOR=x86_64
	-DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc
	-DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++
	-DCMAKE_BUILD_TYPE=Release
	-DCMAKE_C_FLAGS_RELEASE="${COMMON_FLAGS_WIN} ${AMD64_FLAGS}"
	-DCMAKE_CXX_FLAGS_RELEASE="${COMMON_FLAGS_WIN} ${AMD64_FLAGS}"
)

if [[ ! -d "libs" ]]; then mkdir -p libs; fi
cd libs

# Clone libs
if [[ ! -d "SDL" ]]; then
	git clone -b 'release-2.26.5' --single-branch --depth 1 https://github.com/libsdl-org/SDL SDL
fi
if [[ ! -d "SDL_image" ]]; then
	git clone -b 'release-2.6.3' --single-branch --depth 1 https://github.com/libsdl-org/SDL_image SDL_image
fi
if [[ ! -d "box2d" ]]; then
	git clone -b 'v2.4.1' --single-branch --depth 1 https://github.com/erincatto/box2d box2d
fi

# Build functions
build_sdl() {
	local build_dir="$_CWD/libs/SDL/$1"
	local install_dir="$_CWD/libs/SDL/$2"
	local flags=("${!3}")

	if [[ "$2" == "install_mingw" ]]; then
		# Windows libs are DLL, not static
		local ISDLL=ON
		local ISNDLL=OFF
	else
		local ISDLL=OFF
		local ISNDLL=ON
	fi

	if [[ " ${TARGETS[@]} " =~ " clean " ]]; then
		echo "rm -rf $build_dir $install_dir"
		rm -rf "$build_dir" "$install_dir"
	elif [[ ! -d "$build_dir" ]]; then
		TFLAGS="-DSDL_STATIC=$ISNDLL -DSDL_SHARED=$ISDLL -DBUILD_SHARED_LIBS=$ISDLL -DSDL_DYNAMIC_API=$ISDLL"
		mkdir -p "$build_dir" && cd "$build_dir"
		cmake "${flags[@]}" $TFLAGS -DCMAKE_INSTALL_PREFIX="$install_dir" ..
		make -j$(nproc)
		make install
		cd "$_CWD/libs"
	fi
}

# Requires SDL built
build_sdl_image() {
	local build_dir="$_CWD/libs/SDL_image/$1"
	local install_dir="$_CWD/libs/SDL_image/$2"
	local flags=("${!3}")

	if [[ "$2" == "install_mingw" ]]; then
		# Windows libs are DLL, not static
		local ISDLL=ON
		local ISNDLL=OFF
	else
		local ISDLL=OFF
		local ISNDLL=ON
	fi

	if [[ " ${TARGETS[@]} " =~ " clean " ]]; then
		echo "rm -rf $build_dir $install_dir"
		rm -Rf "$build_dir" "$install_dir"
	elif [[ ! -d "$build_dir" ]]; then
		if [[ ! -d "$_CWD/libs/SDL/$2" ]]; then
			echo "SDL_image error: SDL not built" >&2
			exit 1
		fi

		TFLAGS="-DSDL_STATIC=$ISNDLL -DBUILD_SHARED_LIBS=$ISDLL -DSDL_DYNAMIC_API=$ISDLL"
		mkdir -p "$build_dir" && cd "$build_dir"
		cmake "${flags[@]}" $TFLAGS \
			-DSDL2_DIR="$_CWD/libs/SDL/$2/lib/cmake/SDL2" \
			-DCMAKE_PREFIX_PATH="$_CWD/libs/SDL/$2" \
			-DCMAKE_INSTALL_PREFIX="$install_dir" ..
		make -j$(nproc)
		make install
		cd "$_CWD/libs"
	fi
}

build_box2d() {
	local build_dir="$_CWD/libs/box2d/$1"
	local install_dir="$_CWD/libs/box2d/$2"
	local flags=("${!3}")

	if [[ "$2" == "install_mingw" ]]; then
		# Windows libs are DLL, not static
		local ISDLL=ON
	else
		local ISDLL=OFF
	fi

	if [[ " ${TARGETS[@]} " =~ " clean " ]]; then
		echo "rm -rf $build_dir $install_dir"
		rm -rf "$build_dir" "$install_dir"
	elif [[ ! -d "$build_dir" ]]; then
		TFLAGS="-DBUILD_SHARED_LIBS=$ISDLL -DBOX2D_BUILD_UNIT_TESTS=OFF -DBOX2D_BUILD_DOCS=OFF"
		mkdir -p "$build_dir" && cd "$build_dir"
		cmake "${flags[@]}" $TFLAGS -DCMAKE_INSTALL_PREFIX="$install_dir" ..
		make -j$(nproc)
		make install
		cd "$_CWD/libs"
	fi
}

if [[ " ${TARGETS[@]} " =~ " amd64 " ]]; then
build_sdl "build_amd64" "install_amd64" FLAGS_AMD64[@]; fi
if [[ " ${TARGETS[@]} " =~ " arm64 " ]]; then
build_sdl "build_arm64" "install_arm64" FLAGS_ARM64[@]; fi
if [[ " ${TARGETS[@]} " =~ " mingw " ]]; then
build_sdl "build_mingw" "install_mingw" FLAGS_MINGW[@]; fi

if [[ " ${TARGETS[@]} " =~ " amd64 " ]]; then
build_sdl_image "build_amd64" "install_amd64" FLAGS_AMD64[@]; fi
if [[ " ${TARGETS[@]} " =~ " arm64 " ]]; then
build_sdl_image "build_arm64" "install_arm64" FLAGS_ARM64[@]; fi
if [[ " ${TARGETS[@]} " =~ " mingw " ]]; then
build_sdl_image "build_mingw" "install_mingw" FLAGS_MINGW[@]; fi

if [[ " ${TARGETS[@]} " =~ " amd64 " ]]; then
build_box2d "build_amd64" "install_amd64" FLAGS_AMD64[@]; fi
if [[ " ${TARGETS[@]} " =~ " arm64 " ]]; then
build_box2d "build_arm64" "install_arm64" FLAGS_ARM64[@]; fi
if [[ " ${TARGETS[@]} " =~ " mingw " ]]; then
build_box2d "build_mingw" "install_mingw" FLAGS_MINGW[@]; fi

cd "$_CWD"
