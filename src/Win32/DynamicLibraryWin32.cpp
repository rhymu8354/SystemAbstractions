/**
 * @file DynamicLibraryWin32.cpp
 *
 * This module contains the Windows implementation of the
 * SystemAbstractions::DynamicLibrary class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include <Windows.h>

#include <assert.h>
#include <StringExtensions/StringExtensions.hpp>
#include <SystemAbstractions/DynamicLibrary.hpp>
#include <vector>

namespace SystemAbstractions {

    /**
     * This structure contains the private properties of the
     * DynamicLibrary class.
     */
    struct DynamicLibraryImpl {
        HMODULE libraryHandle = NULL;
    };

    DynamicLibrary::~DynamicLibrary() {
        if (_impl == nullptr) {
            return;
        }
        Unload();
    }

    DynamicLibrary::DynamicLibrary(DynamicLibrary&& other) noexcept
        : _impl(std::move(other._impl))
    {
    }

    DynamicLibrary& DynamicLibrary::operator=(DynamicLibrary&& other) noexcept {
        _impl = std::move(other._impl);
        return *this;
    }

    DynamicLibrary::DynamicLibrary()
        : _impl(new DynamicLibraryImpl())
    {
    }

    bool DynamicLibrary::Load(const std::string& path, const std::string& name) {
        Unload();
        std::vector< char > originalPath(MAX_PATH);
        (void)GetCurrentDirectoryA((DWORD)originalPath.size(), &originalPath[0]);
        (void)SetCurrentDirectoryA(path.c_str());
        const auto library = StringExtensions::sprintf("%s/%s.dll", path.c_str(), name.c_str());
        _impl->libraryHandle = LoadLibraryA(library.c_str());
        (void)SetCurrentDirectoryA(&originalPath[0]);
        return (_impl->libraryHandle != NULL);
    }

    void DynamicLibrary::Unload() {
        if (_impl->libraryHandle != NULL) {
            (void)FreeLibrary(_impl->libraryHandle);
            _impl->libraryHandle = NULL;
        }
    }

    void* DynamicLibrary::GetProcedure(const std::string& name) {
        return GetProcAddress(_impl->libraryHandle, name.c_str());
    }

    std::string DynamicLibrary::GetLastError() {
        return StringExtensions::sprintf("%lu", (unsigned long)::GetLastError());
    }

}
