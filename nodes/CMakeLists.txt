cmake_minimum_required(VERSION 3.26)

project(enzoOps)

find_package(Boost REQUIRED)
# eigen (math)
find_package (Eigen3 REQUIRED NO_MODULE)
# manifold (boolean nodes)
find_package(manifold CONFIG REQUIRED)

# Every builtin node compiles into one library, so startup performs a single
# library load no matter how many nodes ship.
file(GLOB NODE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*/src/*.cpp")

set(libName enzoOps)
add_library(${libName} SHARED ${NODE_SOURCES})

target_link_libraries(${libName} PRIVATE enzoEngine Qt6::Core Qt6::Widgets Qt6::SvgWidgets Qt6::OpenGLWidgets Boost::filesystem Boost::system glm::glm Eigen3::Eigen TBB::tbb manifold::manifold libDaScript)

# packaging
install(TARGETS ${libName}
    LIBRARY DESTINATION ${ENZO_LIB_DIR}
    ARCHIVE DESTINATION ${ENZO_LIB_DIR}
)

# The node folders ship alongside the binaries, with the sources stripped out
# since the library already carries them.
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${ENZO_NODES_DIR}
    PATTERN "src" EXCLUDE
    PATTERN "CMakeLists.txt" EXCLUDE
)

target_include_directories(${libName} PUBLIC
    ../src
    ../extern/icecream-cpp/include
    ${DASLANG_DIR}/include
)
