cmake_minimum_required(VERSION 3.26)

project(enzoOps1)

find_package(Boost REQUIRED)
# eigen (math)
find_package (Eigen3 3.3 REQUIRED NO_MODULE)
# manifold (boolean ops)
find_package(manifold CONFIG REQUIRED)

set(libName enzoOps1)
add_library(${libName} SHARED
    RegisterPlugin.cpp
    GopTransform.cpp
    GopHouse.cpp
    GopGeometryImport.cpp
    GopGrid.cpp
    GopSineWave.cpp
    GopOceanSurface.cpp
    GopMerge.cpp
    GopCopyToPoints.cpp
    GopPath.cpp
    GopCamera.cpp
    GopDelete.cpp
    GopExtrude.cpp
    GopCube.cpp
    GopBoolean.cpp
    GopCircle.cpp
    GopSweep.cpp
)

target_link_libraries(${libName} PRIVATE enzoEngine Qt6::Core Qt6::Widgets Qt6::SvgWidgets Qt6::OpenGLWidgets Boost::filesystem Boost::system glm::glm Eigen3::Eigen TBB::tbb manifold::manifold libDaScript)

# packaging
install(TARGETS ${libName}
    LIBRARY DESTINATION ${ENZO_LIB_DIR}
    ARCHIVE DESTINATION ${ENZO_LIB_DIR}
)


MESSAGE("CURRENT SOURCE DIR" ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(${libName} PUBLIC
    ../
    .
    ../../extern/icecream-cpp/include
    ${DASLANG_DIR}/include
)
