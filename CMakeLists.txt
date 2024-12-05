cmake_minimum_required(VERSION 3.25)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS_RELEASE_INIT "-Ofast -s -fno-stack-protector -fomit-frame-pointer -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG_INIT "-Wall -Wno-switch -Wno-format-truncation -g -O0")

cmake_policy(SET CMP0079 NEW)

project(SandSaga
	VERSION 0.1.0.0
	DESCRIPTION "Sand falling simulator in C minigame"
	HOMEPAGE_URL ""
	LANGUAGES C
	)

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
		OUTPUT_NAME game_sdl2
)

# ===============================================================
# Dependencies
# Add SDL2 CMake modules
include(cmake/SDL2.cmake)

# ===============================================================
# Other C Modules

add_subdirectory( "src/assets" )
add_subdirectory( "src/graphics" )
add_subdirectory( "src/engine" )

target_link_libraries(${PROJECT_NAME}
	graphics
	engine
	assets
	)
