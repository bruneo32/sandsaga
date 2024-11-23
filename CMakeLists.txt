cmake_minimum_required(VERSION 3.25)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS_RELEASE "-O2 -s -DNDEBUG -fno-stack-protector -fomit-frame-pointer")
set(CMAKE_C_FLAGS_DEBUG "-Wall -Wno-switch -Wno-format-truncation -g -O0")

cmake_policy(SET CMP0079 NEW)

project(sandsaga
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

# ===============================================================
# CPack Configuration
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "Bruno Castro Garcia")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

if (DIST_BUILDER STREQUAL "windows")
	set(CPACK_PACKAGE_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/windows")
	set(CPACK_PACKAGE_GENERATOR "ZIP")
	set(CPACK_GENERATOR "ZIP")
	set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
	install(TARGETS ${PROJECT_NAME} DESTINATION sandsaga)
	install(FILES
		"${CMAKE_SOURCE_DIR}/mingw_libs/SDL2-2.30.9/x86_64-w64-mingw32/bin/SDL2.dll"
		"${CMAKE_SOURCE_DIR}/mingw_libs/SDL2_image-2.8.2/x86_64-w64-mingw32/bin/SDL2_image.dll"
		DESTINATION sandsaga
	)

	# Include CPack after all settings
	include(CPack)
endif()

# DO NOT include CPack is DIST_BUILDER is invalid
