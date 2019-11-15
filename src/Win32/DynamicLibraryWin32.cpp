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
        if (impl_ == nullptr) {
            return;
        }
        Unload();
    }

    DynamicLibrary::DynamicLibrary(DynamicLibrary&& other) noexcept
        : impl_(std::move(other.impl_))
    {
    }

    DynamicLibrary& DynamicLibrary::operator=(DynamicLibrary&& other) noexcept {
        impl_ = std::move(other.impl_);
        return *this;
    }

    DynamicLibrary::DynamicLibrary()
        : impl_(new DynamicLibraryImpl())
    {
    }

    bool DynamicLibrary::Load(const std::string& path, const std::string& name) {
        Unload();
        std::vector< char > originalPath(MAX_PATH);
        (void)GetCurrentDirectoryA((DWORD)originalPath.size(), &originalPath[0]);
        (void)SetCurrentDirectoryA(path.c_str());
        const auto library = StringExtensions::sprintf("%s/%s.dll", path.c_str(), name.c_str());
        impl_->libraryHandle = LoadLibraryA(library.c_str());
        (void)SetCurrentDirectoryA(&originalPath[0]);
        return (impl_->libraryHandle != NULL);
    }

    void DynamicLibrary::Unload() {
        if (impl_->libraryHandle != NULL) {
            (void)FreeLibrary(impl_->libraryHandle);
            impl_->libraryHandle = NULL;
        }
    }

    void* DynamicLibrary::GetProcedure(const std::string& name) {
        return GetProcAddress(impl_->libraryHandle, name.c_str());
    }

    std::string DynamicLibrary::GetLastError() {
        return StringExtensions::sprintf("%lu", (unsigned long)::GetLastError());
    }

}
