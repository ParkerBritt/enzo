# Configures only the Boost libraries enzo names. Boost pulls in whatever those
# depend on by itself.
#
# The library list is a cmake list, so the semicolons have to survive being
# written into a command line. LIST_SEPARATOR carries them across as bars.
set(ENZO_BOOST_LIBRARIES algorithm container_hash dll filesystem signals2 system)
list(JOIN ENZO_BOOST_LIBRARIES | ENZO_BOOST_LIBRARY_ARG)

ExternalProject_Add(boost
    URL ${ENZO_BOOST_URL}
    URL_HASH SHA256=${ENZO_BOOST_SHA256}
    LIST_SEPARATOR |
    CMAKE_ARGS
        ${ENZO_DEP_CMAKE_ARGS}
        -DBOOST_INCLUDE_LIBRARIES:STRING=${ENZO_BOOST_LIBRARY_ARG}
        -DBUILD_SHARED_LIBS:BOOL=ON
        -DBOOST_INSTALL_LAYOUT:STRING=system
)
list(APPEND ENZO_DEPS boost)
