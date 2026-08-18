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

set(ENZO_DEV_LIB_DIR "${CMAKE_SOURCE_DIR}/build/nodes/")
set(ENZO_DEV_NODES_DIR "${CMAKE_SOURCE_DIR}/nodes")

set(ENGINE_SOURCES
    src/Engine/Attribute/Attribute.cpp
    src/Engine/Attribute/Transform.cpp
    src/Engine/Primitives/Primitive.cpp
    src/Engine/Primitives/Mesh.cpp
    src/Engine/Primitives/Camera.cpp
    src/Engine/Selection/PrimPath.cpp
    src/Engine/Core/Path.cpp
    src/Engine/Network/Network.cpp
    src/Engine/Network/NetworkPath.cpp
    src/Engine/Network/NodePacket.cpp
    src/Engine/Network/Node.cpp
    src/Engine/Network/NodeImpl.cpp
    src/Engine/Network/NodeLoader.cpp
    src/Engine/Network/NodeManifest.cpp
    src/Engine/Network/NodeTypeTable.cpp
    src/Engine/Network/CookContext.cpp
    src/Engine/NetworkGraph/NetworkGraph.cpp
    src/Engine/Expression/DasRuntime.cpp
    src/Engine/Expression/ExpressionEngine.cpp
    src/Engine/Expression/ParameterFunctions.cpp
    src/Engine/Parameter/Template.cpp
    src/Engine/Parameter/Parameter.cpp
    src/Engine/Parameter/Ramp.cpp
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

set(ENGINE_INCLUDE_DIRECTORIES
    src
    extern/icecream-cpp/include
    extern/cereal/include
)


# qt
find_package(Qt6 REQUIRED COMPONENTS Core Widgets SvgWidgets OpenGLWidgets OpenGL Quick Qml QuickControls2 ShaderTools)
qt_standard_project_setup()
ADD_DEFINITIONS(-DQT_NO_KEYWORDS)

# glm
find_package(glm REQUIRED)

# eigen (math)
find_package (Eigen3 REQUIRED NO_MODULE)

# tbb
find_package(TBB REQUIRED COMPONENTS tbb)

find_package(Boost REQUIRED COMPONENTS filesystem system)

# cgal
find_package(CGAL REQUIRED COMPONENTS Core)

# manifold (boolean ops)
find_package(manifold CONFIG REQUIRED)

# yaml-cpp (node manifests and themes)
include(FetchContent)
FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG        0.8.0
)
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(yaml-cpp)

# daslang (das scripting language), built from the vendored submodule
set(DASLANG_DIR "${CMAKE_SOURCE_DIR}/extern/daslang")

# Trim daslang to the core libraries enzo links against.
set(DAS_TOOLS_DISABLED ON CACHE BOOL "" FORCE)
set(DAS_TESTS_DISABLED ON CACHE BOOL "" FORCE)
set(DAS_TUTORIAL_DISABLED ON CACHE BOOL "" FORCE)
set(DAS_AOT_EXAMPLES_DISABLED ON CACHE BOOL "" FORCE)
set(DAS_TREE_SITTER_DISABLED ON CACHE BOOL "" FORCE)
set(DAS_GLFW_DISABLED ON CACHE BOOL "" FORCE)
set(DAS_AUDIO_DISABLED ON CACHE BOOL "" FORCE)
set(DAS_STDDLG_DISABLED ON CACHE BOOL "" FORCE)
set(DAS_STBIMAGE_DISABLED ON CACHE BOOL "" FORCE)
set(DAS_PUGIXML_DISABLED ON CACHE BOOL "" FORCE)
set(DAS_HV_DISABLED ON CACHE BOOL "" FORCE)
set(DAS_SQLITE_DISABLED ON CACHE BOOL "" FORCE)

add_subdirectory(${DASLANG_DIR} ${CMAKE_BINARY_DIR}/daslang EXCLUDE_FROM_ALL)

# daslang exposes its public headers only as a private requirement, so the
# include path is added by hand.
list(APPEND ENGINE_INCLUDE_DIRECTORIES ${DASLANG_DIR}/include)

# daslang loads its standard library (daslib) from a folder next to the bin
# directory of the running executable.
set(DAS_DASLIB_DIR "${DASLANG_DIR}/daslib")

# Dev builds stage into build/enzo so the layout mirrors the installed package.
set(ENZO_DEV_STAGE_DIR "${CMAKE_BINARY_DIR}/enzo")

# daslib sits beside the staged bin directory so daslang resolves its standard
# library the same way it does from an install.
file(COPY ${DAS_DASLIB_DIR} DESTINATION ${ENZO_DEV_STAGE_DIR})

# daslang is built without RTTI, so the files that subclass its types match it.
# Otherwise their typeinfo references daslang symbols that were never emitted.
set_source_files_properties(
    src/Engine/Expression/DasRuntime.cpp
    src/Engine/Expression/ParameterFunctions.cpp
    PROPERTIES COMPILE_OPTIONS "-fno-rtti"
)

# The engine is a self contained library with no knowledge of the ui or the
# operator plugins. The ui executable and the plugins both link against it, so
# the whole process shares one engine and one set of its singletons. The engine
# loads the plugins at runtime rather than linking them.
add_library(enzoEngine SHARED ${ENGINE_SOURCES})
target_link_libraries(enzoEngine PUBLIC glm::glm Eigen3::Eigen TBB::tbb Boost::filesystem Boost::system CGAL::CGAL CGAL::CGAL_Core manifold::manifold libDaScript yaml-cpp::yaml-cpp)
target_include_directories(enzoEngine PUBLIC ${ENGINE_INCLUDE_DIRECTORIES})
target_compile_definitions(enzoEngine PRIVATE
    ENZO_DEV_LIB_DIR="${ENZO_DEV_LIB_DIR}"
    ENZO_DEV_NODES_DIR="${ENZO_DEV_NODES_DIR}"
)





# QML application
qt_add_executable(${AppExec}
    src/Gui/main.cpp
    src/Gui/Style/ThemeLoader.cpp
    src/Gui/Spreadsheet/AttributeTableModel.cpp
    src/Gui/Spreadsheet/PrimitiveTreeModel.cpp
    src/Gui/Spreadsheet/SpreadsheetViewModel.cpp
    src/Gui/Network/NodeListModel.cpp
    src/Gui/Network/EdgeListModel.cpp
    src/Gui/Network/NetworkViewModel.cpp
    src/Gui/Parameters/ParameterItem.cpp
    src/Gui/Parameters/ParametersViewModel.cpp
    src/Gui/Controllers/SceneController.cpp

    # --- Viewport placeholder (throwaway, replaced by the Diligent rebuild).
    # Self-contained under src/Gui/Viewport; delete this block to remove it.
    src/Gui/Viewport/ViewportViewModel.cpp
    src/Gui/Viewport/GLMesh.cpp
    src/Gui/Viewport/GLGrid.cpp
    src/Gui/Viewport/GLPoints.cpp
    src/Gui/Viewport/GLCamera.cpp
    src/Gui/Viewport/GLCameraPrim.cpp
    # --- end viewport placeholder
)

# Register Constants.qml and ParameterClipboard.qml as singletons
set_source_files_properties(src/Gui/Style/Constants.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)
set_source_files_properties(src/Gui/Components/ParameterClipboard.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(${AppExec}
    URI Enzo
    VERSION 1.0
    SOURCES
        src/Gui/Style/Theme.cpp
        src/Gui/Network/NodeLinkLayer.cpp
        src/Gui/Parameters/RampCurveItem.cpp
        src/Gui/Viewport/ViewportItem.cpp
    QML_FILES
        src/Gui/App.qml
        src/Gui/Utils.js
        src/Gui/Style/Constants.qml
        src/Gui/Components/Icon.qml
        src/Gui/Components/IconButton.qml
        src/Gui/Components/Panel.qml
        src/Gui/Components/Slider.qml
        src/Gui/Components/PopupList.qml
        src/Gui/Components/Dropdown.qml
        src/Gui/Components/Stepper.qml
        src/Gui/Components/Tooltip.qml
        src/Gui/Components/PlaceholderDialog.qml
        src/Gui/Components/ParameterClipboard.qml
        src/Gui/Components/ParameterContextMenu.qml
        src/Gui/Components/ParameterContextMenuAction.qml
        src/Gui/Components/ParameterContextMenuRow.qml
        src/Gui/Components/ParameterContextMenuTrigger.qml
        src/Gui/MenuBar/AppMenuBar.qml
        src/Gui/MenuBar/MenuBar.qml
        src/Gui/MenuBar/MenuPopup.qml
        src/Gui/Parameters/ParametersPanel.qml
        src/Gui/Parameters/ParameterRow.qml
        src/Gui/Parameters/ParameterList.qml
        src/Gui/Parameters/FloatSlider.qml
        src/Gui/Parameters/IntSlider.qml
        src/Gui/Parameters/Toggle.qml
        src/Gui/Parameters/VectorParameter.qml
        src/Gui/Parameters/StringParameter.qml
        src/Gui/Parameters/DropdownParameter.qml
        src/Gui/Parameters/Group.qml
        src/Gui/Parameters/RampEditor.qml
        src/Gui/Network/Network.qml
        src/Gui/Network/Node.qml
        src/Gui/Network/NodeLinkController.qml
        src/Gui/Network/TabMenu.qml
        src/Gui/Spreadsheet/Spreadsheet.qml
        src/Gui/Spreadsheet/SpreadsheetHeader.qml
        src/Gui/Spreadsheet/AttributeTable.qml
        src/Gui/Spreadsheet/PrimitivesTree.qml
        src/Gui/Spreadsheet/ModeControl.qml
        src/Gui/Viewport/Viewport.qml
    RESOURCES
        # Declares Constants.qml a singleton for the directory import.
        src/Gui/Style/qmldir
        # Declares ParameterClipboard.qml a singleton for the directory import.
        src/Gui/Components/qmldir
)

qt6_add_shaders(${AppExec} "enzoShaders"
    PREFIX
    "/"
    BASE
    "src/Gui/Shaders"
    FILES
    "src/Gui/Shaders/NetworkDots.frag"
)

# Dev builds load QML from the source tree so edits reload without a rebuild.
# Packaged builds omit this and load the module compiled into the binary.
target_compile_definitions(${AppExec} PRIVATE
    ENZO_QML_SOURCE_DIR="${CMAKE_SOURCE_DIR}/src/Gui"
    ENZO_DEV_STATIC_DIR="${CMAKE_SOURCE_DIR}/static"
)

set_target_properties(${AppExec} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ENZO_DEV_STAGE_DIR}/bin"
)

target_link_libraries(${AppExec} PRIVATE enzoEngine Qt6::Quick Qt6::QuickControls2 Qt6::OpenGL)
target_include_directories(${AppExec} PRIVATE
    ${ENGINE_INCLUDE_DIRECTORIES}
    # Folders holding QML_ELEMENT types, whose headers the generated type
    # registration includes by bare file name.
    src/Gui/Style
    src/Gui/Network
    src/Gui/Parameters
    src/Gui/Viewport
)


# --- packaging ---
set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_SOURCE_DIR}/cmake/CPackProjectConfig.cmake)

set(CPACK_GENERATOR "RPM;TGZ")

set(ENZO_BIN_DIR ${CMAKE_PROJECT_NAME}/bin CACHE STRING "Binary install directory")
set(ENZO_LIB_DIR ${CMAKE_PROJECT_NAME}/lib CACHE STRING "Library install directory")
set(ENZO_NODES_DIR ${CMAKE_PROJECT_NAME}/nodes CACHE STRING "Node folder install directory")


set_target_properties(${AppExec} PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib;$ORIGIN/../../lib"
)

include(CPack)

# cpack_add_component("runtimes")
# cpack_add_component("tgz-runtimes")

install(TARGETS ${AppExec} enzoEngine
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

# Ship daslib as a sibling of enzo/bin so the runtime finds the standard library.
install(DIRECTORY ${DAS_DASLIB_DIR}
    DESTINATION ${CMAKE_PROJECT_NAME}
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



# compile nodes
add_subdirectory(nodes)

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
        tests/main-tests.cpp
        tests/ExpressionEngineTests.cpp
        tests/ParameterTests.cpp
        tests/NetworkGraphTests.cpp
        tests/RampTests.cpp
        tests/SerializerTests.cpp
        tests/UndoTests.cpp
        tests/AttributeTests.cpp
        tests/NetworkTests.cpp
        tests/SelectionTests.cpp
        tests/MeshTests.cpp
        tests/BooleanUtilsTests.cpp
        tests/PathTests.cpp
        tests/NetworkPathTests.cpp
        tests/NodeManifestTests.cpp
        tests/NodeLoaderTests.cpp
        tests/GeometryImportTests.cpp
        tests/TransformNodeTests.cpp
    )
    target_link_libraries(${TestExec} PRIVATE enzoEngine Catch2::Catch2WithMain)

    # Stage the test binary next to daslib so daslang resolves its standard
    # library the same way the app does.
    set_target_properties(${TestExec} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${ENZO_DEV_STAGE_DIR}/bin"
    )

    # benchmarks
    add_executable(${BenchExec}
        tests/Benchmarks.cpp
    )
    target_link_libraries(${BenchExec} PRIVATE enzoEngine Catch2::Catch2WithMain)

    # GUI unit tests, Qt and yaml-cpp only, no engine
    add_executable(enzoGuiTests
        tests/gui/ThemeLoaderTests.cpp
        src/Gui/Style/ThemeLoader.cpp
    )
    target_link_libraries(enzoGuiTests PRIVATE Qt6::Gui yaml-cpp::yaml-cpp Catch2::Catch2WithMain)
    target_include_directories(enzoGuiTests PRIVATE src)
    target_compile_definitions(enzoGuiTests PRIVATE
        ENZO_DEV_STATIC_DIR="${CMAKE_SOURCE_DIR}/static"
    )
endif()

