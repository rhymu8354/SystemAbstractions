# CMakeLists.txt for MockDynamicLibrary
#
# © 2018 by Richard Walters

cmake_minimum_required(VERSION 3.8)
set(This MockDynamicLibrary)

set(Sources
    src/MockDynamicLibrary.cpp
)

add_library(${This} SHARED ${Sources})
set_target_properties(${This} PROPERTIES
    FOLDER TestHelpers
)

target_link_libraries(${This} PUBLIC
)
