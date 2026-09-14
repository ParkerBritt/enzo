# Turns the installed tree into RPM, DEB, tar.gz, MSI, EXE installer and ZIP
# packages, and gathers the Qt and third party libraries the application loads
# at runtime.

set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_SOURCE_DIR}/cmake/CPackProjectConfig.cmake)

if(WIN32)
    set(CPACK_GENERATOR "WIX;NSIS;ZIP")
else()
    set(CPACK_GENERATOR "RPM;DEB;TGZ")
endif()

set(CPACK_PACKAGE_NAME enzo)
set(CPACK_PACKAGE_VENDOR "Enzo Software")
set(CPACK_PACKAGE_CONTACT "Parker Britt <parker@parkerbritt.com>")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://enzo3d.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Node based procedural 3D modeling framework")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Enzo")
# The WiX generator only accepts a licence with a .txt or .rtf extension.
configure_file(${CMAKE_SOURCE_DIR}/LICENSE ${CMAKE_BINARY_DIR}/LICENSE.txt COPYONLY)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_BINARY_DIR}/LICENSE.txt)
set(CPACK_PACKAGE_EXECUTABLES ${AppExec} "Enzo 3D")
set(CPACK_STRIP_FILES ON)

# Names the archive and package files after the platform they were built for,
# so artifacts from every job can sit in one release.
if(WIN32)
    set(CPACK_SYSTEM_NAME windows-${CMAKE_SYSTEM_PROCESSOR})
else()
    set(CPACK_SYSTEM_NAME linux-${CMAKE_SYSTEM_PROCESSOR})
endif()

# Brands the Windows executable and both installers with the app icon and the
# header and welcome images.
if(WIN32)
    set(ENZO_WINDOWS_PACKAGING_DIR "${CMAKE_SOURCE_DIR}/packaging/windows")
    set(ENZO_WINDOWS_ICON "${ENZO_WINDOWS_PACKAGING_DIR}/enzo.ico")

    configure_file(${ENZO_WINDOWS_PACKAGING_DIR}/enzo.rc.in ${CMAKE_BINARY_DIR}/enzo.rc)
    target_sources(${AppExec} PRIVATE ${CMAKE_BINARY_DIR}/enzo.rc)

    # Offers to run the launcher when the installer finishes, and shows its
    # icon in the installed programs list.
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "${AppExec}.exe")
    # Writes the backslash as four, since CPack copies the value into its
    # config file without escaping it.
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\${AppExec}.exe")

    # Doubles the backslashes of the NSIS image paths, since makensis needs
    # native paths and CPack copies the values into its config file unescaped.
    file(TO_NATIVE_PATH "${ENZO_WINDOWS_PACKAGING_DIR}" ENZO_NSIS_PACKAGING_DIR)
    string(REPLACE "\\" "\\\\" ENZO_NSIS_PACKAGING_DIR "${ENZO_NSIS_PACKAGING_DIR}")

    set(CPACK_NSIS_IGNORE_LICENSE_PAGE ON)
    set(CPACK_NSIS_MUI_ICON "${ENZO_NSIS_PACKAGING_DIR}\\\\enzo.ico")
    set(CPACK_NSIS_MUI_UNIICON "${ENZO_NSIS_PACKAGING_DIR}\\\\enzo.ico")
    set(CPACK_NSIS_MUI_HEADERIMAGE "${ENZO_NSIS_PACKAGING_DIR}\\\\nsis-header.bmp")
    set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${ENZO_NSIS_PACKAGING_DIR}\\\\nsis-welcome.bmp")
    set(CPACK_NSIS_MUI_UNWELCOMEFINISHPAGE_BITMAP "${ENZO_NSIS_PACKAGING_DIR}\\\\nsis-welcome.bmp")

    set(CPACK_WIX_PRODUCT_ICON "${ENZO_WINDOWS_ICON}")
    set(CPACK_WIX_UI_BANNER "${ENZO_WINDOWS_PACKAGING_DIR}/wix-banner.bmp")
    set(CPACK_WIX_UI_DIALOG "${ENZO_WINDOWS_PACKAGING_DIR}/wix-dialog.bmp")
    set(CPACK_WIX_UI_REF EnzoUI_InstallDir)
    set(CPACK_WIX_EXTRA_SOURCES "${ENZO_WINDOWS_PACKAGING_DIR}/EnzoUI.wxs")
endif()

set_target_properties(${AppExec} PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib;$ORIGIN/../../lib"
)

include(CPack)

# Copies the Qt libraries, plugins and qml modules the app reaches at install
# time, along with a qt.conf pointing at them.
qt_generate_deploy_qml_app_script(
    TARGET ${AppExec}
    OUTPUT_SCRIPT ENZO_QT_DEPLOY_SCRIPT
)

# Sets the destinations in the install script itself, since Qt reads them when
# the deployment runs rather than when it is generated.
install(CODE "
    set(QT_DEPLOY_BIN_DIR \"${ENZO_BIN_DIR}\")
    set(QT_DEPLOY_LIB_DIR \"${ENZO_LIB_DIR}\")
    set(QT_DEPLOY_LIBEXEC_DIR \"${ENZO_APP_DIR}libexec\")
    set(QT_DEPLOY_PLUGINS_DIR \"${ENZO_APP_DIR}plugins\")
    set(QT_DEPLOY_QML_DIR \"${ENZO_APP_DIR}qml\")
    set(QT_DEPLOY_TRANSLATIONS_DIR \"${ENZO_APP_DIR}translations\")
")
install(SCRIPT ${ENZO_QT_DEPLOY_SCRIPT})

# Installs enzo's own binaries after the Qt deployment, so they land with their
# install runpath.
install(TARGETS ${AppExec} enzoEngine
    RUNTIME_DEPENDENCY_SET enzoRuntime
    RUNTIME DESTINATION ${ENZO_BIN_DIR}
    LIBRARY DESTINATION ${ENZO_LIB_DIR}
    ARCHIVE DESTINATION ${ENZO_LIB_DIR}
)

# Installs the remaining shared dependencies by reading the imports of the
# installed binaries, since windeployqt brings only Qt.
if(WIN32)
    set(ENZO_DEP_BIN_DIRS "")
    foreach(prefix IN LISTS CMAKE_PREFIX_PATH)
        list(APPEND ENZO_DEP_BIN_DIRS "${prefix}/bin")
    endforeach()

    install(RUNTIME_DEPENDENCY_SET enzoRuntime
        DIRECTORIES ${ENZO_DEP_BIN_DIRS}
        # Enzo's own libraries have their own install rules. The api-ms and
        # ext-ms names are Windows api sets rather than files on disk.
        PRE_EXCLUDE_REGEXES "^enzo.*\\.dll$" "api-ms-" "ext-ms-"
        POST_EXCLUDE_REGEXES "[Ss]ystem32"
        RUNTIME DESTINATION ${ENZO_BIN_DIR}
    )
endif()


# Puts the launcher on PATH, so the desktop entry and a shell both reach it by
# name.
if(UNIX)
    install(CODE "
        set(root \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}\")
        file(MAKE_DIRECTORY \"\${root}/bin\")
        file(REMOVE \"\${root}/bin/${AppExec}\")
        file(CREATE_LINK \"../${ENZO_BIN_DIR}/${AppExec}\" \"\${root}/bin/${AppExec}\" SYMBOLIC)
    ")
endif()

# Installs the desktop entry, metadata and icon Linux desktops read.
if(UNIX)
    install(FILES flatpak/com.enzo3d.Enzo.desktop
        DESTINATION share/applications
    )
    install(FILES flatpak/com.enzo3d.Enzo.metainfo.xml
        DESTINATION share/metainfo
    )
    install(FILES static/icons/icon-main-white-square.svg
        DESTINATION share/icons/hicolor/scalable/apps
        RENAME com.enzo3d.Enzo.svg
    )
endif()

# Installs the licence into the folder holding bin and lib.
cmake_path(SET ENZO_APP_ROOT NORMALIZE "${ENZO_APP_DIR}.")
install(FILES ${CMAKE_BINARY_DIR}/LICENSE.txt
    DESTINATION ${ENZO_APP_ROOT}
)

# The fonts and icons the application reads at runtime. The theme is compiled
# into the binary.
install(DIRECTORY static/
    DESTINATION ${ENZO_APP_DIR}share
    PATTERN "theme" EXCLUDE
)

# Installs daslib beside enzo/bin, where the runtime looks for the standard
# library.
install(DIRECTORY ${DAS_DASLIB_DIR}/
    DESTINATION ${ENZO_APP_DIR}daslib
)

