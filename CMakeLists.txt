cmake_minimum_required(VERSION 3.25)
cmake_policy(SET CMP0079 NEW)

set(CMAKE_C_STANDARD 99)

# Box2D somehow does not work with less than -O2 level
set(CMAKE_C_FLAGS_RELEASE "-O2 -s -DNDEBUG -fno-stack-protector -fomit-frame-pointer")
set(CMAKE_C_FLAGS_DEBUG "-O2 -Wall -Wno-switch -Wno-format-truncation -g")
set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(sandsaga
	VERSION 0.1.0.0
	DESCRIPTION "Sand falling simulator in C minigame"
	HOMEPAGE_URL ""
	LANGUAGES C CXX
	)

set(PROJECT_MAINTAINER "Bruno Castro <bruneo32b@gmail.com>")

link_libraries(
	m # Link lib-math
	)

# ===============================================================
# Main source
file(GLOB MAIN_SRC "src/main.c")
add_executable(${PROJECT_NAME} ${MAIN_SRC})

set_target_properties(
	${PROJECT_NAME}
	PROPERTIES
		OUTPUT_NAME sandsaga
)

# ===============================================================
# Dependencies
include(cmake/SDL2.cmake)
include(cmake/box2d.cmake)

# ===============================================================
# Other C Modules
add_subdirectory( "src/physics" )
add_subdirectory( "src/assets" )
add_subdirectory( "src/graphics" )
add_subdirectory( "src/engine" )

# ===============================================================
# CPack Configuration
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "Bruno Castro Garcia")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_CONTACT ${PROJECT_MAINTAINER})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

include("cmake/cpack/cpack_linux.cmake")
include("cmake/cpack/cpack_windows.cmake")
