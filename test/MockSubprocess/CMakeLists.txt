# CMakeLists.txt for MockSubprocess
#
# © 2018 by Richard Walters

cmake_minimum_required(VERSION 3.8)
set(This MockSubprocess)

set(Sources
    src/main.cpp
)

add_executable(${This} ${Sources})
set_target_properties(${This} PROPERTIES
    FOLDER TestHelpers
)

target_link_libraries(${This} PUBLIC
    SystemAbstractions
)
