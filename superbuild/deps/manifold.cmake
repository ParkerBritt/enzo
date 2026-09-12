# Manifold provides the mesh boolean. Enzo includes only manifold.h, so the 2D
# cross section and the Clipper2 it needs are left out.

ExternalProject_Add(manifold
    URL ${ENZO_MANIFOLD_URL}
    URL_HASH SHA256=${ENZO_MANIFOLD_SHA256}
    DEPENDS tbb
    LIST_SEPARATOR |
    CMAKE_ARGS
        ${ENZO_DEP_CMAKE_ARGS}
        -DMANIFOLD_CROSS_SECTION:BOOL=OFF
        -DMANIFOLD_PAR:BOOL=ON
        -DMANIFOLD_TEST:BOOL=OFF
        -DMANIFOLD_PYBIND:BOOL=OFF
        -DMANIFOLD_DOWNLOADS:BOOL=OFF
        # Only the engine calls manifold, so it links in rather than shipping as
        # its own library.
        -DBUILD_SHARED_LIBS:BOOL=OFF
)
list(APPEND ENZO_DEPS manifold)
