cmake_minimum_required(VERSION 3.7)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS_RELEASE_INIT "-Ofast -s -fno-stack-protector -fomit-frame-pointer -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG_INIT "-Wall -Wno-switch -Wno-format-truncation -g -O0")

project(CSand C)

# ===============================================================
# Dependencies
# Add SDL2 CMake modules
add_definitions(-D_REENTRANT)

set(SDL2_INCLUDE_DIRS "/usr/include/SDL2")
set(SDL2_LIBRARIES "-lSDL2")

set(SDL2_IMAGE_LIBRARIES "-lSDL2_image")

include_directories(
	${SDL2_INCLUDE_DIRS}
	)

link_libraries(
	${SDL2_LIBRARIES}
	${SDL2_IMAGE_LIBRARIES}
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

target_compile_definitions(${PROJECT_NAME} PRIVATE SDL_DISABLE_IMMINTRIN_H)

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
