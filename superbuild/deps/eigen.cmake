# Eigen is header only, so the install is the headers and the package config.
ExternalProject_Add(eigen
    URL ${ENZO_EIGEN_URL}
    URL_HASH SHA256=${ENZO_EIGEN_SHA256}
    LIST_SEPARATOR |
    CMAKE_ARGS
        ${ENZO_DEP_CMAKE_ARGS}
        -DEIGEN_BUILD_DOC:BOOL=OFF
        -DEIGEN_BUILD_TESTING:BOOL=OFF
)
list(APPEND ENZO_DEPS eigen)
