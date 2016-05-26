/**
 * @file DynamicLibraryPosix.cpp
 *
 * This module contains the POSIX implementation of the
 * SystemAbstractions::DynamicLibrary class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "../DynamicLibrary.hpp"
#include "../StringExtensions.hpp"

#include <assert.h>
#include <dlfcn.h>

namespace SystemAbstractions {

    /**
     * This structure contains the private properties of the
     * DynamicLibrary class.
     */
    struct DynamicLibraryImpl {
        void* libraryHandle = NULL;
    };

    DynamicLibrary::DynamicLibrary()
        : _impl(new DynamicLibraryImpl())
    {
    }


    DynamicLibrary::DynamicLibrary(DynamicLibrary&& other)
        : _impl(std::move(other._impl))
    {
    }

    DynamicLibrary::DynamicLibrary(std::unique_ptr< DynamicLibraryImpl >&& impl)
        : _impl(std::move(impl))
    {
    }

    DynamicLibrary::~DynamicLibrary() {
        if (_impl == nullptr) {
            return;
        }
        Unload();
    }

    DynamicLibrary& DynamicLibrary::operator=(DynamicLibrary&& other) {
        assert(this != &other);
        _impl = std::move(other._impl);
        return *this;
    }

    bool DynamicLibrary::Load(const std::string& path, const std::string& name) {
        Unload();
        const auto library = SystemAbstractions::sprintf("%s/lib%s.so", path.c_str(), name.c_str());
        _impl->libraryHandle = dlopen(library.c_str(), RTLD_LAZY);
        return (_impl->libraryHandle != NULL);
    }

    void DynamicLibrary::Unload() {
        if (_impl->libraryHandle != NULL) {
            (void)dlclose(_impl->libraryHandle);
            _impl->libraryHandle = NULL;
        }
    }

    void* DynamicLibrary::GetProcedure(const std::string& name) {
        return dlsym(_impl->libraryHandle, name.c_str());
    }

}
