# CMakeLists.txt for SystemAbstractionsTests
#
# © 2018 by Richard Walters

cmake_minimum_required(VERSION 3.8)
set(This SystemAbstractionsTests)

set(Sources
    src/DiagnosticsSenderTests.cpp
    src/DirectoryMonitorTests.cpp
    src/FileTests.cpp
    src/StringExtensionsTests.cpp
    src/StringFileTests.cpp
)

add_executable(${This} ${Sources})
set_target_properties(${This} PROPERTIES
    FOLDER Tests
)

target_include_directories(${This} PRIVATE ..)

target_link_libraries(${This} PUBLIC
    gtest_main
    SystemAbstractions
)

add_test(
    NAME ${This}
    COMMAND ${This}
)
