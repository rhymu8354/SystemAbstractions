#ifndef SYSTEM_ABSTRACTIONS_DYNAMIC_LIBRARY_IMPL_HPP
#define SYSTEM_ABSTRACTIONS_DYNAMIC_LIBRARY_IMPL_HPP

/**
 * @file DynamicLibraryImpl.hpp
 *
 * This module contains the POSIX implementation of the
 * SystemAbstractions::DynamicLibraryImpl structure.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include <string>

namespace SystemAbstractions {

    /**
     * This structure contains the private properties of the
     * DynamicLibrary class.
     */
    struct DynamicLibraryImpl {
        // Properties

        void* libraryHandle = NULL;

        // Methods

        static std::string GetDynamicLibraryFileExtension();
    };

}

#endif /* SYSTEM_ABSTRACTIONS_DYNAMIC_LIBRARY_IMPL_HPP */
