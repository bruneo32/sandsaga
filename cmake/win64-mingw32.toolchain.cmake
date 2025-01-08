# Toolchain file: windows-toolchain.cmake

set(CMAKE_SYSTEM_NAME Windows)
set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--subsystem,windows")

set(WINDOWS_COMPAT_FLAGS "-fopenmp -m64 -funroll-loops -msha")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${WINDOWS_COMPAT_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${WINDOWS_COMPAT_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})

# Set the path to the cross-compiler
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_Fortran_COMPILER ${TOOLCHAIN_PREFIX}-gfortran)
set(CMAKE_RC_COMPILER ${TOOLCHAIN_PREFIX}-windres)

# Set the path to the Windows libraries
set(CMAKE_FIND_ROOT_PATH /usr/${TOOLCHAIN_PREFIX})

# Ensure the cross-compiling root path is searched for headers and libraries
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

link_libraries("-lmingw32")

# Set library paths for mingw
set(SDL2_INCLUDE_DIRS "${SDL2_PATH}/include/SDL2")
set(SDL2_IMAGE_INCLUDE_DIRS "${SDL2_IMAGE_PATH}/include/SDL2")
set(BOX2D_INCLUDE_DIRS "${BOX2D_PATH}/include")

link_directories(
	${SDL2_PATH}/lib
	${SDL2_IMAGE_PATH}/lib
	${BOX2D_PATH}/lib
	)
