# Add SDL2 CMake modules
add_definitions(-D_REENTRANT)

include_directories(
	"${SDL_PATH}/include/SDL2"
	"${SDL_IMAGE_PATH}/include/SDL2"
	)

target_link_directories(${PROJECT_NAME} PRIVATE
	"${SDL_PATH}/lib"
	"${SDL_IMAGE_PATH}/lib"
	)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# Windows libs are DLL, not static
	target_link_libraries(${PROJECT_NAME}
		-Wl,-Bdynamic
		SDL2main SDL2
		SDL2_image
		)
else()
	target_link_libraries(${PROJECT_NAME}
		-Wl,-Bstatic
		SDL2main SDL2
		SDL2_image
		-Wl,-Bdynamic
		)
endif()
