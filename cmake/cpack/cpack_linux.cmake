if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(CPACK_PACKAGE_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/linux")
	set(CPACK_PACKAGE_GENERATOR "TGZ;DEB;RPM")
	set(CPACK_GENERATOR "TGZ;DEB;RPM")

	# Avoid conflicts between TGZ and system packages
	set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
	set(CPACK_ABSOLUTE_DESTINATION_FILES ON)
	set(CPACK_INSTALL_PREFIX "/")
	set(CPACK_SET_DESTDIR ON)

	# TGZ-specific settings
	set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-Linux-${CMAKE_SYSTEM_PROCESSOR}")

	# DEB-specific settings
	set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
	set(CPACK_DEBIAN_COMPRESSION_TYPE "gzip")
	set(CPACK_DEBIAN_PACKAGE_HOMEPAGE ${PROJECT_HOMEPAGE_URL})
	set(CPACK_DEBIAN_PACKAGE_SECTION "games")
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl2-2.0-0, libsdl2-image-2.0-0, libbox2d2")
	# Architecture
	if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
		set(DEBIAN_ARCH "amd64")
	elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
		set(DEBIAN_ARCH "arm64")
	endif()
	set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${DEBIAN_ARCH})

	# RPM-specific settings
	set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
	set(CPACK_RPM_PACKAGE_LICENSE "GPLv3+")
	set(CPACK_RPM_PACKAGE_GROUP "Games")
	set(CPACK_RPM_PACKAGE_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
	set(CPACK_RPM_PACKAGE_DESCRIPTION ${PROJECT_DESCRIPTION})
	set(CPACK_RPM_PACKAGE_REQUIRES "SDL2 >= 2.0, SDL2_image >= 2.0, box2d >= 2.0")

	# Since we are distributing a stripped version without debug symbols,
	# the build ID links serve no practical purpose for the end user.
	set(CPACK_RPM_SPEC_MORE_DEFINE "
		%define _build_id_links none
		%define _buildhost localhost
	")

	# Install paths
	install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION usr/games)
	install(FILES
		"${CMAKE_SOURCE_DIR}/cmake/extra/sandsaga.desktop"
		DESTINATION usr/share/applications
	)
	install(FILES
		"${CMAKE_SOURCE_DIR}/LICENSE"
		"${CMAKE_SOURCE_DIR}/cmake/extra/icon.png"
		DESTINATION usr/share/games/sandsaga
	)

	# Include CPack after all settings
	include(CPack)
endif()

# DO NOT include CPack is DIST_BUILDER is invalid
