cmake_minimum_required(VERSION 3.8)

set(This SystemAbstractions)
set(CMAKE_CXX_STANDARD 11)

set(SourceExtensions *.c *.cpp *.cxx)
set(HeaderExtensions *.h *.hpp *.hxx)

file(GLOB Sources ${SourceExtensions})
file(GLOB Headers ${HeaderExtensions})

if(MSVC)
    set(TargetFolders Win32)
    add_definitions(
        -DUNICODE -D_UNICODE
        -D_CRT_SECURE_NO_WARNINGS
    )
elseif(APPLE)
    set(TargetFolders Mach Posix)
    list(APPEND SourceExtensions *.m *.mm)
elseif(UNIX)
    set(TargetFolders Linux Posix)
endif()

if(DEFINED TargetFolders)
    foreach(TargetFolder ${TargetFolders})
        foreach(SourceExtension ${SourceExtensions})
            file(GLOB TargetSources
                ${TargetFolder}/${SourceExtension}
            )
            list(APPEND Sources ${TargetSources})
        endforeach(SourceExtension ${SourceExtensions})
        foreach(HeaderExtension ${HeaderExtensions})
            file(GLOB TargetHeaders
                ${TargetFolder}/${HeaderExtension}
            )
            list(APPEND Headers ${TargetHeaders})
        endforeach(HeaderExtension ${SourceExtensions})
    endforeach(TargetFolder ${TargetFolders})
endif(DEFINED TargetFolders)

add_library(${This} STATIC ${Sources} ${Headers})
set_target_properties(${This} PROPERTIES
    FOLDER Libraries
)

if(APPLE)
    find_library(CF CoreFoundation)
    target_link_libraries(${This} ${CF})
endif(APPLE)
if(UNIX)
    target_link_libraries(${This}
        pthread
        dl
    )
endif(UNIX)

target_include_directories(${This} INTERFACE ..)
