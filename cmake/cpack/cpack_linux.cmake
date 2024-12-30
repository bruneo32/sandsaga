if (DIST_BUILDER STREQUAL "linux")
    set(CPACK_PACKAGE_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/linux")
    set(CPACK_PACKAGE_GENERATOR "DEB;TGZ") # Add TGZ for tarball generation
    set(CPACK_GENERATOR "DEB;TGZ")

    # DEB-specific settings
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_DEBIAN_COMPRESSION_TYPE "gzip")
    set(CPACK_DEBIAN_PACKAGE_NAME ${PROJECT_NAME})
    set(CPACK_DEBIAN_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION})
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${PROJECT_MAINTAINER})
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE ${PROJECT_HOMEPAGE_URL})
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
    set(CPACK_PACKAGE_DESCRIPTION ${PROJECT_DESCRIPTION})
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_DEBIAN_PACKAGE_SECTION "games")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsdl2-2.0-0, libsdl2-image-2.0-0, libbox2d2")

    # TGZ-specific settings
    set(CPACK_TGZ_FILE_NAME TAR-DEFAULT)
    set(CPACK_TGZ_PACKAGE_NAME ${PROJECT_NAME})
    set(CPACK_TGZ_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION})
    set(CPACK_TGZ_PACKAGE_DESCRIPTION ${PROJECT_DESCRIPTION})

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
