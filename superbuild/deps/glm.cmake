# GLM is built header only, so nothing links a glm library.
ExternalProject_Add(glm
    URL ${ENZO_GLM_URL}
    URL_HASH SHA256=${ENZO_GLM_SHA256}
    LIST_SEPARATOR |
    CMAKE_ARGS
        ${ENZO_DEP_CMAKE_ARGS}
        -DGLM_BUILD_LIBRARY:BOOL=OFF
        -DGLM_BUILD_TESTS:BOOL=OFF
        -DGLM_BUILD_INSTALL:BOOL=ON
)
list(APPEND ENZO_DEPS glm)
