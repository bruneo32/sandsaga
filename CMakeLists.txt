cmake_minimum_required(VERSION 3.25)
cmake_policy(SET CMP0079 NEW)

set(CMAKE_C_STANDARD 99)

# TODO: test ARM executable
# Box2D somehow does not work with less than -O2 level
set(COMMON_FLAGS "-O2 -march=${PROCESSOR} -mtune=generic -flto")

set(CMAKE_C_FLAGS_RELEASE "${COMMON_FLAGS} -s -DNDEBUG -fno-stack-protector -fomit-frame-pointer")
set(CMAKE_C_FLAGS_DEBUG "${COMMON_FLAGS} -Wall -Wno-switch -Wno-format-truncation -g")
set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(sandsaga
	VERSION 0.0.1.1
	LANGUAGES C CXX
	DESCRIPTION "Sand falling simulator in C minigame"
	HOMEPAGE_URL "https://github.com/bruneo32/sandsaga"
	)

link_libraries(
	m	# libm (math)
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
set(CPACK_PACKAGE_DESCRIPTION ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "Bruno Castro Garcia <bruneo32b@gmail.com>")
set(CPACK_PACKAGE_CONTACT ${CPACK_PACKAGE_VENDOR})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

include("cmake/cpack/cpack_linux.cmake")
include("cmake/cpack/cpack_windows.cmake")
