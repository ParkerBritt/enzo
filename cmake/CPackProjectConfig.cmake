# Sets the packaging options for whichever generator CPack is running. CPack
# reads this file once the generator is chosen, so CPACK_GENERATOR holds a
# single name.

# The distribution packages land under /usr, which puts the launcher symlink on
# PATH and the desktop entry, icon and metainfo where the desktop reads them.
if(CPACK_GENERATOR STREQUAL "RPM" OR CPACK_GENERATOR STREQUAL "DEB")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
endif()

if(CPACK_GENERATOR STREQUAL "RPM")
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
    # Every library the application needs ships with it, so the generated
    # requires list would name build host packages the target does not have.
    set(CPACK_RPM_PACKAGE_AUTOREQ OFF)
    # The directories the package shares with the base system.
    set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
        /usr/share/applications
        /usr/share/icons
        /usr/share/icons/hicolor
        /usr/share/icons/hicolor/scalable
        /usr/share/icons/hicolor/scalable/apps
        /usr/share/metainfo
    )
endif()

if(CPACK_GENERATOR STREQUAL "DEB")
    set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
    # Matches the architecture names dpkg uses rather than the kernel's.
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
endif()

if(CPACK_GENERATOR STREQUAL "WIX")
    # Identifies the product across releases so an install replaces the
    # previous version instead of sitting beside it.
    set(CPACK_WIX_UPGRADE_GUID "6F3A6E1C-6D5E-4C1B-9E2A-1B7C4D0F8A32")
    set(CPACK_WIX_PROPERTY_ARPHELPLINK "${CPACK_PACKAGE_HOMEPAGE_URL}")
    set(CPACK_WIX_ROOT_FEATURE_TITLE "Enzo 3D")
endif()
