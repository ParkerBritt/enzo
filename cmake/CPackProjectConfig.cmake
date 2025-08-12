if (CPACK_GENERATOR MATCHES "RPM")
    # rpm specific config

    if(UNIX)
        set(CPACK_PACKAGING_INSTALL_PREFIX "/opt")
    endif(UNIX)

elseif(CPACK_GENERATOR MATCHES "TGZ")
    # tgz specific config

endif()

