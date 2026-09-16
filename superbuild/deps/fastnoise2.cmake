# FastNoise2 provides the noise nodes. It fetches its own FastSIMD at configure
# time, so that version is left to FastNoise2.

ExternalProject_Add(fastnoise2
    URL ${ENZO_FASTNOISE2_URL}
    URL_HASH SHA256=${ENZO_FASTNOISE2_SHA256}
    LIST_SEPARATOR |
    CMAKE_ARGS
        ${ENZO_DEP_CMAKE_ARGS}
        -DFASTNOISE2_TOOLS:BOOL=OFF
        -DFASTNOISE2_TESTS:BOOL=OFF
        -DFASTNOISE2_UTILITY:BOOL=OFF
        # Only the nodes call FastNoise2, so it links in rather than shipping as
        # its own library.
        -DBUILD_SHARED_LIBS:BOOL=OFF
)
list(APPEND ENZO_DEPS fastnoise2)
