cmake_minimum_required(VERSION 3.7)

project(CSand C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS_RELEASE_INIT "-O3 --strip-all -fnoseparate-code -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG_INIT "-Wall -g -O0")

# ===============================================================
# Dependencies

# Add SDL2 CMake modules
find_package(SDL2 REQUIRED)
# find_package(SDL2_image REQUIRED)

# I couldn't get SDL2_image to work, so I'm just manually adding the requirement
set(SDL2_IMAGE_LIBRARIES " -lSDL2_image")
string(STRIP "${SDL2_IMAGE_LIBRARIES}" SDL2_IMAGE_LIBRARIES)

add_definitions(-D_REENTRANT)
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

add_subdirectory(	"src/graphics"	)
add_subdirectory(	"src/engine"	)

target_link_libraries(${PROJECT_NAME}
	graphics
	engine
	# assets
	)
