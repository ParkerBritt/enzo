# Sets the packaging options for whichever generator CPack is running. CPack
# reads this file once the generator is chosen, so CPACK_GENERATOR holds a
# single name.

if(CPACK_GENERATOR STREQUAL "RPM")
    # Installs a self contained tree rather than merging into the system
    # prefixes.
    if(UNIX)
        set(CPACK_PACKAGING_INSTALL_PREFIX "/opt")
    endif()
endif()
