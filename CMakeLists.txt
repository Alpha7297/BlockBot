cmake_minimum_required(VERSION 3.16)

project(BlockBot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)
qt_standard_project_setup()

qt_add_big_resources(BlockBotResources resource.qrc)

qt_add_executable(BlockBot
    main.cpp
    ui/ArchivePage.cpp
    ui/ArchivePage.h
    ui/App.cpp
    ui/App.h
    ui/AudioManager.cpp
    ui/AudioManager.h
    ui/hint.cpp
    ui/hint.h
    ui/AppGraphicsView.h
    ui/MainWindow.cpp
    ui/MainWindow.h
    ui/LevelChoosePage.cpp
    ui/LevelChoosePage.h
    ui/UiConstants.h
    ui/Widgets.h
    message/Message.cpp
    message/Message.h
    level/Level.cpp
    level/Level.h
    level/LevelConstants.h
    level/MazeGenerator.h
    level/mazeGernerator.cpp
    ui/SettingsDialog.cpp
    ui/SettingsDialog.h
    ui/SaveCrypto.cpp
    ui/SaveCrypto.h
    ui/parameter.h
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
    ${BlockBotResources}
)

target_link_libraries(BlockBot PRIVATE Qt6::Widgets Qt6::Multimedia)
if(WIN32)
    target_link_libraries(BlockBot PRIVATE Crypt32)
endif()
