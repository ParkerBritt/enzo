# TBB is the parallel backend for both enzo and manifold, so the two share this
# one copy of the scheduler.
ExternalProject_Add(tbb
    URL ${ENZO_TBB_URL}
    URL_HASH SHA256=${ENZO_TBB_SHA256}
    LIST_SEPARATOR |
    CMAKE_ARGS
        ${ENZO_DEP_CMAKE_ARGS}
        -DTBB_TEST:BOOL=OFF
        -DTBB_EXAMPLES:BOOL=OFF
        -DTBB_STRICT:BOOL=OFF
        -DTBBMALLOC_BUILD:BOOL=OFF
)
list(APPEND ENZO_DEPS tbb)
