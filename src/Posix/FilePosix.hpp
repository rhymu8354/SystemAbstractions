#ifndef SYSTEM_ABSTRACTIONS_FILE_POSIX_HPP
#define SYSTEM_ABSTRACTIONS_FILE_POSIX_HPP

/**
 * @file FilePosix.hpp
 *
 * This module declares the POSIX implementation of the
 * SystemAbstractions::FileImpl structure.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include <string>
#include <SystemAbstractions/File.hpp>

namespace SystemAbstractions {

    /**
     * This is the POSIX-specific state for the File class.
     */
    struct File::Platform {
        /**
         * This is the operating-system handle to the underlying file.
         */
        FILE* handle = NULL;
    };

    /**
     * This is a helper function that returns the home directory path
     * of the current user.
     *
     * @return
     *     The home directory path of the current user is returned.
     */
    std::string GetUserHomeDirectoryPath();

}

#endif /* SYSTEM_ABSTRACTIONS_FILE_POSIX_HPP */
