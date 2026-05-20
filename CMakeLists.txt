cmake_minimum_required(VERSION 3.16)

project(helloQt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(helloQt
    main.cpp
    ui/App.cpp
    message/Message.cpp
    core/Runtime.cpp
    core/Block.cpp
    core/Ops.cpp
    core/Value.cpp
)

target_link_libraries(helloQt PRIVATE Qt6::Widgets)

add_executable(test_calc
    test.cpp
    core/Block.cpp
    core/Ops.cpp
    core/Value.cpp
)

add_custom_target(run_test
    COMMAND test_calc
    DEPENDS test_calc
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
