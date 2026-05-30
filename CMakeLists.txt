cmake_minimum_required(VERSION 3.16)

project(BlockBot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(BlockBot
    main.cpp
    ui/App.cpp
    ui/App.h
    ui/AppGraphicsView.h
    ui/MainWindow.cpp
    ui/MainWindow.h
    ui/levelchoosepage.cpp
    ui/levelchoosepage.h
    ui/UiConstants.h
    ui/Widgets.h
    message/Message.cpp
    message/Message.h
    level/Level.cpp
    level/Level.h
    level/LevelConstants.h
    level/MazeGenerator.h
    level/mazeGernerator.cpp
    settings/SettingsDialog.cpp
    settings/SettingsDialog.h
    settings/parameter.h
    tale/TaleWindow.cpp
    tale/TaleWindow.h
    core/Runtime.cpp
    core/Runtime.h
    core/Block.cpp
    core/Block.h
    core/Ops.cpp
    core/Ops.h
    core/Value.cpp
    core/Value.h
)

target_link_libraries(BlockBot PRIVATE Qt6::Widgets)
