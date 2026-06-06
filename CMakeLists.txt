cmake_minimum_required(VERSION 3.26)

# set vars
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
# set exec names
set(AppExec enzoGui)
set(TestExec tests)
set(BenchExec bench)
set(CMAKE_AUTORCC ON)

# setup project
project(enzo)

set(ENZO_DEV_LIB_DIR "${CMAKE_SOURCE_DIR}/build/src/OpDefs/")

set(ENGINE_SOURCES
    src/Engine/Attribute/Attribute.cpp
    src/Engine/Primitives/Primitive.cpp
    src/Engine/Primitives/Mesh.cpp
    src/Engine/Primitives/Camera.cpp
    src/Engine/Selection/PrimPath.cpp
    src/Engine/Network/NodePacket.cpp
    src/Engine/Network/GeometryOperator.cpp
    src/Engine/Network/GeometryConnection.cpp
    src/Engine/Network/GeometryOpDef.cpp
    src/Engine/Network/OperatorTable.cpp
    src/Engine/Network/Context.cpp
    src/Engine/Parameter/Template.cpp
    src/Engine/Parameter/Parameter.cpp
    src/Engine/Parameter/NodeParameter.cpp
    src/Engine/Parameter/Default.cpp
    src/Engine/Parameter/Range.cpp
    src/Engine/Parameter/PrmName.cpp
    src/Engine/Network/NetworkManager.cpp
    src/Engine/Network/UpdateLock.cpp
    src/Engine/UndoRedo/UndoStack.cpp
    src/Engine/Serializer/Serializer.cpp
    src/Engine/Selection/Selection.cpp
    src/Engine/Selection/SelectionComponent.cpp
    src/Engine/Selection/IndexSet.cpp
    src/Engine/GeometryAlgorithms/MeshUtils.cpp
    src/Engine/GeometryAlgorithms/MeshShapes.cpp
    src/Engine/GeometryAlgorithms/BooleanUtils.cpp
    src/Engine/Util/UnionFind.cpp
)

set(GUI_SOURCES
    static/resources.qrc
    src/Gui/main.cpp
    src/Gui/Interface.cpp
    src/Gui/IconRegistry.cpp
    src/Gui/Icons/Builtin.cpp
    src/Gui/UtilWidgets/Splitter.cpp
    src/Gui/UtilWidgets/BoolSwitch.cpp
    src/Gui/UtilWidgets/Slider.cpp
    src/Gui/UtilWidgets/Dropdown.cpp
    src/Gui/GeometrySpreadsheetPanel/GeometrySpreadsheetPanel.cpp
    src/Gui/GeometrySpreadsheetPanel/GeometrySpreadsheetMenuBar.cpp
    src/Gui/GeometrySpreadsheetPanel/AttributeSpreadsheetModel.cpp
    src/Gui/GeometrySpreadsheetPanel/PrimitiveTreeModel.cpp
    src/Gui/GeometrySpreadsheetPanel/PrimitiveTreeItem.cpp
    src/Gui/Viewport/Viewport.cpp
    src/Gui/Viewport/ViewportGLWidget.cpp
    src/Gui/Viewport/GLCamera.cpp
    src/Gui/Viewport/GLMesh.cpp
    src/Gui/Viewport/GLGrid.cpp
    src/Gui/Viewport/GLPoints.cpp
    src/Gui/Viewport/GLCameraPrim.cpp
    src/Gui/Viewport/ViewportOverlay.cpp
    src/Gui/Network/NetworkGraphicsView.cpp
    src/Gui/Network/NetworkGraphicsScene.cpp
    src/Gui/Network/NetworkPanel.cpp
    src/Gui/Network/NodeGraphic.cpp
    src/Gui/Network/SocketGraphic.cpp
    src/Gui/Network/NodeEdgeGraphic.cpp
    src/Gui/Network/FloatingEdgeGraphic.cpp
    src/Gui/Network/DisplayFlagButton.cpp
    src/Gui/Network/NodeIconGraphic.cpp
    src/Gui/Network/TabMenu.cpp
    src/Gui/ParametersPanel/ParametersPanel.cpp
    src/Gui/Parameters/Parameter.cpp
    src/Gui/Parameters/FloatSliderParm.cpp
    src/Gui/Parameters/IntSliderParm.cpp
    src/Gui/Parameters/BoolSwitchParm.cpp
    src/Gui/Parameters/BoolIconParm.cpp
    src/Gui/Parameters/BoolIconSlashParm.cpp
    src/Gui/Parameters/BoolParm.cpp
    src/Gui/Parameters/XYZParm.cpp
    src/Gui/Parameters/StringParm.cpp
    src/Gui/Parameters/DropdownParm.cpp
    src/Gui/Parameters/GroupParm.cpp
    src/Gui/HeaderBar/HeaderBar.cpp
    src/Gui/Panels/Panel.cpp
    src/Gui/Panels/CornerOverlay.cpp
)
set(ENGINE_INCLUDE_DIRECTORIES
    src
    extern/icecream-cpp/include
    extern/cereal/include
)


# qt
find_package(Qt6 REQUIRED COMPONENTS Core Widgets SvgWidgets OpenGLWidgets)
qt_standard_project_setup()
ADD_DEFINITIONS(-DQT_NO_KEYWORDS)

# glm
find_package(glm REQUIRED)

# eigen (math)
find_package (Eigen3 3.3 REQUIRED NO_MODULE)

# tbb
find_package(TBB REQUIRED COMPONENTS tbb)

find_package(Boost REQUIRED COMPONENTS filesystem system)

# cgal
find_package(CGAL REQUIRED COMPONENTS Core)

# manifold (boolean ops)
find_package(manifold CONFIG REQUIRED)



qt_add_executable(${AppExec}
    ${GUI_SOURCES}
    ${ENGINE_SOURCES}
)

target_compile_definitions(${AppExec} PRIVATE ENZO_DEV_LIB_DIR="${ENZO_DEV_LIB_DIR}")
target_compile_definitions(${AppExec} PRIVATE ENZO_DEV_ICONS_DIR="${CMAKE_SOURCE_DIR}/static/icons")

# TODO: turn this on when you're ready to handle incomplete enums
# target_compile_options(${AppExec} PRIVATE -Werror=switch)


# --- packaging ---
set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_SOURCE_DIR}/cmake/CPackProjectConfig.cmake)

set(CPACK_GENERATOR "RPM;TGZ")

set(ENZO_BIN_DIR ${CMAKE_PROJECT_NAME}/bin CACHE STRING "Binary install directory")
set(ENZO_LIB_DIR ${CMAKE_PROJECT_NAME}/lib CACHE STRING "Library install directory")


set_target_properties(${AppExec} PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib;$ORIGIN/../../lib"
)

include(CPack)

# cpack_add_component("runtimes")
# cpack_add_component("tgz-runtimes")

install(TARGETS ${AppExec}
    RUNTIME DESTINATION ${ENZO_BIN_DIR}
    LIBRARY DESTINATION ${ENZO_LIB_DIR}
    ARCHIVE DESTINATION ${ENZO_LIB_DIR}
    # COMPONENT "runtimes"
)

# desktop integration (Flatpak / Linux desktop)
install(FILES flatpak/org.enzosoftware.Enzo.desktop
    DESTINATION share/applications
)
install(FILES flatpak/org.enzosoftware.Enzo.metainfo.xml
    DESTINATION share/metainfo
)
install(FILES static/icons/icon-main-white-square.svg
    DESTINATION share/icons/hicolor/scalable/apps
    RENAME org.enzosoftware.Enzo.svg
)

install(DIRECTORY static/icons/
    DESTINATION ${CMAKE_PROJECT_NAME}/share/icons
    FILES_MATCHING PATTERN "*.svg"
)

# install(TARGETS ${AppExec}
#     RUNTIME_DEPENDENCY_SET app_deps
#     RUNTIME DESTINATION ${ENZO_BIN_DIR}
#     LIBRARY DESTINATION ${ENZO_LIB_DIR}
#     ARCHIVE DESTINATION ${ENZO_LIB_DIR}
#     COMPONENT "tgz-runtimes"
# )

# install(RUNTIME_DEPENDENCY_SET app_deps
#     DIRECTORIES "${CMAKE_BINARY_DIR}"
#     PRE_EXCLUDE_REGEXES
#     "libc.so.*" "libm.so.*" "libpthread.so.*" "librt.so.*" "libdl.so.*"
#     "libgcc_s.so.*" "libstdc++.so.*"
#     DESTINATION ${ENZO_LIB_DIR}
#     COMPONENT "tgz-runtimes"
# )

# install(
#     DIRECTORY /usr/lib64/qt6
#     DESTINATION ${CMAKE_PROJECT_NAME}
#     COMPONENT "tgz-runtimes"
# )

# install(
#     DIRECTORY /usr/lib64/
#     DESTINATION ${ENZO_LIB_DIR}
#     COMPONENT "tgz-runtimes"
#     FILES_MATCHING PATTERN "*Qt*.so*"
# )

# file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/packaging/qt.conf
# "[Paths]
# Plugins = ../qt6/plugins
# Libraries = ../lib
# ")

# install(
#     FILES ${CMAKE_CURRENT_BINARY_DIR}/packaging/qt.conf
#     DESTINATION ${ENZO_BIN_DIR}
#     COMPONENT "tgz-runtimes"
# )




# set(CPACK_RPM_PACKAGE_REQUIRES "openssl >= 1.1.1, zlib >= 1.2.11")

# --- end packaging


target_link_libraries(${AppExec} PRIVATE Qt6::Core Qt6::Widgets Qt6::SvgWidgets Qt6::OpenGLWidgets glm::glm Eigen3::Eigen TBB::tbb Boost::filesystem Boost::system CGAL::CGAL CGAL::CGAL_Core manifold::manifold)
target_include_directories(${AppExec} PUBLIC
    ${ENGINE_INCLUDE_DIRECTORIES}
)

# compile nodes
add_subdirectory(src/OpDefs)

# tests
option(ENZO_BUILD_TESTS "Build tests and benchmarks" ON)
if(ENZO_BUILD_TESTS)
    Include(FetchContent)

    FetchContent_Declare(
      Catch2
      GIT_REPOSITORY https://github.com/catchorg/Catch2.git
      GIT_TAG        v3.8.1 # or a later release
    )

    FetchContent_MakeAvailable(Catch2)

    add_executable(${TestExec}
        ${ENGINE_SOURCES}
        tests/main-tests.cpp
        tests/OperatorTests.cpp
        tests/NetworkTests.cpp
        tests/SelectionTests.cpp
        tests/MeshTests.cpp
        tests/BooleanUtilsTests.cpp
    )
    target_link_libraries(${TestExec} PRIVATE Catch2::Catch2WithMain Eigen3::Eigen Qt6::Core TBB::tbb Boost::filesystem Boost::system CGAL::CGAL CGAL::CGAL_Core manifold::manifold)
    target_compile_definitions(${TestExec} PRIVATE UNIT_TEST)
    target_include_directories(${TestExec} PUBLIC
        ${ENGINE_INCLUDE_DIRECTORIES}
        ${BOOST_INCLUDE_DIRS}
    )
    target_compile_definitions(${TestExec} PRIVATE ENZO_DEV_LIB_DIR="${ENZO_DEV_LIB_DIR}")

    # benchmarks
    add_executable(${BenchExec}
        ${ENGINE_SOURCES}

        tests/Benchmarks.cpp
    )
    target_link_libraries(${BenchExec} PRIVATE Catch2::Catch2WithMain Eigen3::Eigen Qt6::Core TBB::tbb Boost::filesystem Boost::system CGAL::CGAL CGAL::CGAL_Core manifold::manifold)
    target_compile_definitions(${BenchExec} PRIVATE UNIT_TEST)
    target_include_directories(${BenchExec} PUBLIC
        ${ENGINE_INCLUDE_DIRECTORIES}
    )
    target_compile_definitions(${BenchExec} PRIVATE ENZO_DEV_LIB_DIR="${ENZO_DEV_LIB_DIR}")
endif()

