# Eigen is header only, so the install is the headers and the package config.
ExternalProject_Add(eigen
    URL ${ENZO_EIGEN_URL}
    URL_HASH SHA256=${ENZO_EIGEN_SHA256}
    LIST_SEPARATOR |
    CMAKE_ARGS
        ${ENZO_DEP_CMAKE_ARGS}
        -DEIGEN_BUILD_DOC:BOOL=OFF
        -DEIGEN_BUILD_TESTING:BOOL=OFF
        # Eigen's blas subdirectory enables Fortran whenever it finds a
        # compiler, and nothing there is built. Declaring none skips it.
        -DCMAKE_Fortran_COMPILER:FILEPATH=NOTFOUND
)
list(APPEND ENZO_DEPS eigen)
