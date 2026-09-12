# Downloads Qt as an official prebuilt. aqtinstall does the downloading from its
# own virtual environment, so the host only needs python.
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# The Qt modules that sit outside the base install.
set(ENZO_QT_MODULES qtshadertools)

set(ENZO_AQT_VENV ${CMAKE_BINARY_DIR}/aqt-venv)

# ENZO_QT_ARCH is the name aqtinstall asks for and ENZO_QT_DIR is the folder it
# writes, which drops the host prefix.
if(WIN32)
    set(ENZO_QT_HOST windows)
    set(ENZO_QT_ARCH win64_msvc2022_64)
    set(ENZO_QT_DIR msvc2022_64)
    set(ENZO_AQT_PYTHON ${ENZO_AQT_VENV}/Scripts/python.exe)
else()
    set(ENZO_QT_HOST linux)
    set(ENZO_QT_ARCH linux_gcc_64)
    set(ENZO_QT_DIR gcc_64)
    set(ENZO_AQT_PYTHON ${ENZO_AQT_VENV}/bin/python)
endif()

# Unpacks into its own versioned folder rather than the shared dependency
# prefix, so ENZO_QT_PREFIX is a second place for everything else to search.
set(ENZO_QT_ROOT "${ENZO_DEPS_PREFIX}/qt")
set(ENZO_QT_PREFIX "${ENZO_QT_ROOT}/${ENZO_QT_VERSION}/${ENZO_QT_DIR}")

ExternalProject_Add(qt
    DOWNLOAD_COMMAND
        ${Python3_EXECUTABLE} -m venv ${ENZO_AQT_VENV}
    COMMAND
        ${ENZO_AQT_PYTHON} -m pip install --quiet --disable-pip-version-check
        aqtinstall==${ENZO_AQTINSTALL_VERSION}
    COMMAND
        ${ENZO_AQT_PYTHON} -m aqt install-qt
        ${ENZO_QT_HOST} desktop ${ENZO_QT_VERSION} ${ENZO_QT_ARCH}
        --modules ${ENZO_QT_MODULES}
        --outputdir ${ENZO_QT_ROOT}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    USES_TERMINAL_DOWNLOAD ON
)
list(APPEND ENZO_DEPS qt)
