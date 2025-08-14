MESSAGE(STATUS "GENERATOR: ${CPACK_GENERATOR}")
if (${CPACK_GENERATOR} STREQUAL "RPM")
    # rpm specific config

    MESSAGE(STATUS "RPM")

    set(CPACK_RPM_COMPONENT_INSTALL ON)
    set(CPACK_COMPONENTS_ALL "runtimes")
    if(UNIX)
        set(CPACK_PACKAGING_INSTALL_PREFIX "/opt")
    endif(UNIX)

elseif(${CPACK_GENERATOR} STREQUAL "TGZ")
    # tgz specific config
    MESSAGE(STATUS "TGZ")

    set(CPACK_TGZ_COMPONENT_INSTALL ON)
    set(CPACK_COMPONENTS_ALL "tgz-runtimes")

endif()

