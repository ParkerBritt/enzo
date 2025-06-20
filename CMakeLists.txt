cmake_minimum_required(VERSION 3.10)

# set vars
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# set exec names
set(AppExec enzoGui)
set(TestExec tests)

# setup project
project(enzo_project)


# qt
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
qt_standard_project_setup()


qt_add_executable(${AppExec}
    src/gui/main.cpp
    src/gui/Interface.cpp
    src/gui/viewport/Viewport.cpp
    src/gui/network/NetworkGraphicsView.cpp
    src/gui/network/NetworkGraphicsScene.cpp
    src/gui/network/Network.cpp
    src/gui/network/NodeGraphic.cpp
    src/gui/network/SocketGraphic.cpp
    src/gui/network/NodeEdgeGraphic.cpp
    src/gui/network/FloatingEdgeGraphic.cpp
)

target_link_libraries(${AppExec} PRIVATE Qt6::Core Qt6::Widgets)
target_include_directories(${AppExec} PUBLIC src)

# tests
add_executable(${TestExec} tests/main-tests.cpp)
find_package(Catch2 3 REQUIRED)
target_link_libraries(${TestExec} PRIVATE Catch2::Catch2WithMain)
