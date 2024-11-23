# Add SDL2 CMake modules
add_definitions(-D_REENTRANT)

if (NOT DEFINED SDL2_INCLUDE_DIRS)
	set(SDL2_INCLUDE_DIRS "/usr/include/SDL2")
endif()
if (NOT DEFINED SDL2_IMAGE_INCLUDE_DIRS)
	set(SDL2_IMAGE_INCLUDE_DIRS "/usr/include/SDL2")
endif()

set(SDL2_LIBRARIES "-lSDL2main -lSDL2")
set(SDL2_IMAGE_LIBRARIES "-lSDL2_image")

include_directories(
	${SDL2_INCLUDE_DIRS}
	${SDL2_IMAGE_INCLUDE_DIRS}
	)

link_libraries(
	${SDL2_LIBRARIES}
	${SDL2_IMAGE_LIBRARIES}
	)

target_compile_definitions(${PROJECT_NAME} PRIVATE SDL_DISABLE_IMMINTRIN_H)
