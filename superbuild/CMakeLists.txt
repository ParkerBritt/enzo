# Builds every dependency into one shared prefix, then configures enzo against
# it.

include(ExternalProject)

set(ENZO_DEPS_PREFIX "${CMAKE_BINARY_DIR}/deps"
    CACHE PATH "Where the superbuild installs its dependencies")

set_directory_properties(PROPERTIES EP_BASE ${CMAKE_BINARY_DIR}/superbuild)

# Collects each dependency as it is added, so enzo can depend on the whole set.
set(ENZO_DEPS)

# Include Qt first, since the folder it unpacks into is the second prefix
# everything below searches.
include(deps/qt.cmake)

# The settings shared by the dependencies and by enzo, so one compiler, language
# standard and ABI covers the whole prefix.
#
# Joins the two search prefixes with a bar, which LIST_SEPARATOR turns back into
# a semicolon in each sub project.
set(ENZO_TOOLCHAIN_CMAKE_ARGS
    -DCMAKE_PREFIX_PATH:STRING=${ENZO_DEPS_PREFIX}|${ENZO_QT_PREFIX}
    -DCMAKE_CXX_STANDARD:STRING=20
    -DCMAKE_CXX_STANDARD_REQUIRED:BOOL=ON
    -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
)

# Forwards the toolchain settings that were actually chosen.
foreach(setting IN ITEMS
    CMAKE_BUILD_TYPE
    CMAKE_C_COMPILER
    CMAKE_CXX_COMPILER
    CMAKE_TOOLCHAIN_FILE
    CMAKE_MAKE_PROGRAM
    CMAKE_MSVC_RUNTIME_LIBRARY)
    if(${setting})
        list(APPEND ENZO_TOOLCHAIN_CMAKE_ARGS -D${setting}:STRING=${${setting}})
    endif()
endforeach()

# What every dependency is configured with.
set(ENZO_DEP_CMAKE_ARGS
    ${ENZO_TOOLCHAIN_CMAKE_ARGS}
    -DCMAKE_INSTALL_PREFIX:PATH=${ENZO_DEPS_PREFIX}
    -DBUILD_TESTING:BOOL=OFF
)

# The libraries end up beside each other in the packaged tree, so they look for
# their siblings next to themselves rather than in the prefix they were built in.
if(UNIX)
    list(APPEND ENZO_DEP_CMAKE_ARGS -DCMAKE_INSTALL_RPATH:STRING=$ORIGIN)
endif()

include(deps/tbb.cmake)
include(deps/boost.cmake)
include(deps/manifold.cmake)
include(deps/eigen.cmake)
include(deps/glm.cmake)
include(deps/argparse.cmake)

# Builds enzo last, into a normal cmake build folder that ninja can be run in
# directly.
ExternalProject_Add(enzo
    SOURCE_DIR ${CMAKE_SOURCE_DIR}
    BINARY_DIR ${CMAKE_BINARY_DIR}/enzo-build
    DEPENDS ${ENZO_DEPS}
    LIST_SEPARATOR |
    CMAKE_ARGS
        ${ENZO_TOOLCHAIN_CMAKE_ARGS}
        -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_INSTALL_PREFIX}
        -DENZO_SUPERBUILD:BOOL=OFF
    BUILD_ALWAYS ON
    INSTALL_COMMAND ""
    USES_TERMINAL_CONFIGURE ON
    USES_TERMINAL_BUILD ON
)
