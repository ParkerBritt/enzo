# Argparse is header only, and gives the gui executable its command line.
ExternalProject_Add(argparse
    URL ${ENZO_ARGPARSE_URL}
    URL_HASH SHA256=${ENZO_ARGPARSE_SHA256}
    LIST_SEPARATOR |
    CMAKE_ARGS
        ${ENZO_DEP_CMAKE_ARGS}
        -DARGPARSE_BUILD_TESTS:BOOL=OFF
        -DARGPARSE_BUILD_SAMPLES:BOOL=OFF
)
list(APPEND ENZO_DEPS argparse)
