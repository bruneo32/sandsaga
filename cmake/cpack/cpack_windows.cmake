if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(CPACK_PACKAGE_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/windows")
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_Windows_x86_64")
	set(CPACK_PACKAGE_GENERATOR "ZIP")
	set(CPACK_GENERATOR "ZIP")
	set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)

	install(TARGETS ${PROJECT_NAME} DESTINATION sandsaga)
	install(FILES
		"${CMAKE_SOURCE_DIR}/LICENSE"
		"${CMAKE_SOURCE_DIR}/mingw_libs/SDL2-2.26.5/x86_64-w64-mingw32/bin/SDL2.dll"
		"${CMAKE_SOURCE_DIR}/mingw_libs/SDL2_image-2.6.3/x86_64-w64-mingw32/bin/SDL2_image.dll"
		"${CMAKE_SOURCE_DIR}/mingw_libs/box2d/install_mingw/bin/libbox2d.dll"
		"/usr/lib/gcc/x86_64-w64-mingw32/12-win32/libstdc++-6.dll"
		"/usr/lib/gcc/x86_64-w64-mingw32/12-win32/libgcc_s_seh-1.dll"
		DESTINATION sandsaga
	)

	# Windows resource file (icon, metadata, etc)
	set(WINRES_RC ${CMAKE_BINARY_DIR}/windows.rc)
	configure_file(
		${CMAKE_SOURCE_DIR}/cmake/extra/windows.rc.in
		${WINRES_RC}
		@ONLY
	)

    # Add the .rc file as a source
    target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/windows.rc)

	# Include CPack after all settings
	include(CPack)
endif()

# DO NOT include CPack is DIST_BUILDER is invalid
