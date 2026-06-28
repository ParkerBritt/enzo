cmake_minimum_required(VERSION 3.26)

# set vars
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
# set exec names
set(AppExec enzoGui)
set(LegacyAppExec enzoLegacyGui)
set(TestExec tests)
set(BenchExec bench)
set(CMAKE_AUTORCC ON)

# setup project
project(enzo)

set(ENZO_DEV_LIB_DIR "${CMAKE_SOURCE_DIR}/build/src/OpDefs/")

set(ENGINE_SOURCES
    src/Engine/Attribute/Attribute.cpp
    src/Engine/Attribute/Transform.cpp
    src/Engine/Primitives/Primitive.cpp
    src/Engine/Primitives/Mesh.cpp
    src/Engine/Primitives/Camera.cpp
    src/Engine/Selection/PrimPath.cpp
    src/Engine/Core/Path.cpp
    src/Engine/Network/NetworkPath.cpp
    src/Engine/Network/NodePacket.cpp
    src/Engine/Network/GeometryOperator.cpp
    src/Engine/Network/GeometryOpDef.cpp
    src/Engine/Network/OperatorTable.cpp
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

set(GUI_SOURCES
    static/resources.qrc
    src/LegacyGui/main.cpp
    src/LegacyGui/Interface.cpp
    src/LegacyGui/IconRegistry.cpp
    src/LegacyGui/Icons/Builtin.cpp
    src/LegacyGui/UtilWidgets/Splitter.cpp
    src/LegacyGui/UtilWidgets/BoolSwitch.cpp
    src/LegacyGui/UtilWidgets/Slider.cpp
    src/LegacyGui/UtilWidgets/Ramp.cpp
    src/LegacyGui/UtilWidgets/PopupList.cpp
    src/LegacyGui/UtilWidgets/Menu.cpp
    src/LegacyGui/UtilWidgets/MenuBar.cpp
    src/LegacyGui/UtilWidgets/Dropdown.cpp
    src/LegacyGui/UtilWidgets/IconButton.cpp
    src/LegacyGui/GeometrySpreadsheetPanel/GeometrySpreadsheetPanel.cpp
    src/LegacyGui/GeometrySpreadsheetPanel/GeometrySpreadsheetMenuBar.cpp
    src/LegacyGui/GeometrySpreadsheetPanel/AttributeSpreadsheetModel.cpp
    src/LegacyGui/GeometrySpreadsheetPanel/PrimitiveTreeModel.cpp
    src/LegacyGui/GeometrySpreadsheetPanel/PrimitiveTreeItem.cpp
    src/LegacyGui/Viewport/Viewport.cpp
    src/LegacyGui/Viewport/ViewportGLWidget.cpp
    src/LegacyGui/Viewport/GLCamera.cpp
    src/LegacyGui/Viewport/GLMesh.cpp
    src/LegacyGui/Viewport/GLGrid.cpp
    src/LegacyGui/Viewport/GLPoints.cpp
    src/LegacyGui/Viewport/GLCameraPrim.cpp
    src/LegacyGui/Viewport/ViewportOverlay.cpp
    src/LegacyGui/Network/NetworkGraphicsView.cpp
    src/LegacyGui/Network/NetworkGraphicsScene.cpp
    src/LegacyGui/Network/NetworkPanel.cpp
    src/LegacyGui/Network/NodeGraphic.cpp
    src/LegacyGui/Network/SocketGraphic.cpp
    src/LegacyGui/Network/NodeEdgeGraphic.cpp
    src/LegacyGui/Network/FloatingEdgeGraphic.cpp
    src/LegacyGui/Network/DisplayFlagButton.cpp
    src/LegacyGui/Network/NodeIconGraphic.cpp
    src/LegacyGui/Network/TabMenu.cpp
    src/LegacyGui/ParametersPanel/ParametersPanel.cpp
    src/LegacyGui/Parameters/Parameter.cpp
    src/LegacyGui/Parameters/FloatSliderParm.cpp
    src/LegacyGui/Parameters/IntSliderParm.cpp
    src/LegacyGui/Parameters/BoolSwitchParm.cpp
    src/LegacyGui/Parameters/BoolIconParm.cpp
    src/LegacyGui/Parameters/BoolIconSlashParm.cpp
    src/LegacyGui/Parameters/BoolParm.cpp
    src/LegacyGui/Parameters/XYZParm.cpp
    src/LegacyGui/Parameters/RampParm.cpp
    src/LegacyGui/Parameters/StringParm.cpp
    src/LegacyGui/Parameters/DropdownParm.cpp
    src/LegacyGui/Parameters/GroupParm.cpp
    src/LegacyGui/Parameters/SpacerParm.cpp
    src/LegacyGui/HeaderBar/HeaderBar.cpp
    src/LegacyGui/Panels/Panel.cpp
    src/LegacyGui/Panels/CornerOverlay.cpp
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
find_package (Eigen3 3.3 REQUIRED NO_MODULE)

# tbb
find_package(TBB REQUIRED COMPONENTS tbb)

find_package(Boost REQUIRED COMPONENTS filesystem system)

# cgal
find_package(CGAL REQUIRED COMPONENTS Core)

# manifold (boolean ops)
find_package(manifold CONFIG REQUIRED)

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
target_link_libraries(enzoEngine PUBLIC glm::glm Eigen3::Eigen TBB::tbb Boost::filesystem Boost::system CGAL::CGAL CGAL::CGAL_Core manifold::manifold libDaScript)
target_include_directories(enzoEngine PUBLIC ${ENGINE_INCLUDE_DIRECTORIES})
target_compile_definitions(enzoEngine PRIVATE ENZO_DEV_LIB_DIR="${ENZO_DEV_LIB_DIR}")



# legacy widgets application
qt_add_executable(${LegacyAppExec}
    ${GUI_SOURCES}
)

target_compile_definitions(${LegacyAppExec} PRIVATE ENZO_DEV_ICONS_DIR="${CMAKE_SOURCE_DIR}/static/icons")

# Stage enzo/bin with daslib as its sibling so dev mirrors the package layout.
set_target_properties(${LegacyAppExec} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ENZO_DEV_STAGE_DIR}/bin"
)
file(COPY ${DAS_DASLIB_DIR} DESTINATION ${ENZO_DEV_STAGE_DIR})

# TODO: turn this on when ready to handle incomplete enums
# target_compile_options(${LegacyAppExec} PRIVATE -Werror=switch)


# QML application
qt_add_executable(${AppExec}
    src/Gui/main.cpp
    src/Gui/Style/Theme.cpp
    src/Gui/Spreadsheet/AttributeTableModel.cpp
    src/Gui/Spreadsheet/PrimitiveTreeModel.cpp
    src/Gui/Spreadsheet/SpreadsheetViewModel.cpp
    src/Gui/Network/NodeListModel.cpp
    src/Gui/Network/EdgeListModel.cpp
    src/Gui/Network/NetworkViewModel.cpp
    src/Gui/Parameters/ParameterItem.cpp
    src/Gui/Parameters/ParametersViewModel.cpp

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

qt_add_qml_module(${AppExec}
    URI Enzo
    VERSION 1.0
    SOURCES
        src/Gui/Network/NodeLinkLayer.cpp
        src/Gui/Viewport/ViewportItem.cpp
    QML_FILES
        src/Gui/App.qml
        src/Gui/Utils.js
        src/Gui/Components/Icon.qml
        src/Gui/Components/Panel.qml
        src/Gui/Components/Slider.qml
        src/Gui/Parameters/ParametersPanel.qml
        src/Gui/Parameters/ParameterRow.qml
        src/Gui/Parameters/FloatSlider.qml
        src/Gui/Parameters/IntSlider.qml
        src/Gui/Parameters/Toggle.qml
        src/Gui/Parameters/VectorField.qml
        src/Gui/Parameters/StringField.qml
        src/Gui/Parameters/Dropdown.qml
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
    ENZO_DEV_ICONS_DIR="${CMAKE_SOURCE_DIR}/static/icons"
    ENZO_DEV_FONTS_DIR="${CMAKE_SOURCE_DIR}/static/fonts"
)

set_target_properties(${AppExec} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ENZO_DEV_STAGE_DIR}/bin"
)

target_link_libraries(${AppExec} PRIVATE enzoEngine Qt6::Quick Qt6::QuickControls2 Qt6::OpenGL)
target_include_directories(${AppExec} PRIVATE
    ${ENGINE_INCLUDE_DIRECTORIES}
    # Folders holding QML_ELEMENT types, whose headers the generated type
    # registration includes by bare file name.
    src/Gui/Network
    src/Gui/Viewport
)


# --- packaging ---
set(CPACK_PROJECT_CONFIG_FILE ${CMAKE_SOURCE_DIR}/cmake/CPackProjectConfig.cmake)

set(CPACK_GENERATOR "RPM;TGZ")

set(ENZO_BIN_DIR ${CMAKE_PROJECT_NAME}/bin CACHE STRING "Binary install directory")
set(ENZO_LIB_DIR ${CMAKE_PROJECT_NAME}/lib CACHE STRING "Library install directory")


set_target_properties(${AppExec} ${LegacyAppExec} PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib;$ORIGIN/../../lib"
)

include(CPack)

# cpack_add_component("runtimes")
# cpack_add_component("tgz-runtimes")

install(TARGETS ${AppExec} ${LegacyAppExec} enzoEngine
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


target_link_libraries(${LegacyAppExec} PRIVATE enzoEngine Qt6::Core Qt6::Widgets Qt6::SvgWidgets Qt6::OpenGLWidgets glm::glm Eigen3::Eigen TBB::tbb Boost::filesystem Boost::system CGAL::CGAL CGAL::CGAL_Core manifold::manifold libDaScript)
target_include_directories(${LegacyAppExec} PUBLIC
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
        tests/main-tests.cpp
        tests/ExpressionEngineTests.cpp
        tests/ParameterTests.cpp
        tests/NetworkGraphTests.cpp
        tests/RampTests.cpp
        tests/SerializerTests.cpp
        tests/UndoTests.cpp
        tests/OperatorTests.cpp
        tests/NetworkTests.cpp
        tests/SelectionTests.cpp
        tests/MeshTests.cpp
        tests/BooleanUtilsTests.cpp
        tests/PathTests.cpp
        tests/NetworkPathTests.cpp
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
endif()

