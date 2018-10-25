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
        int handle = -1;

        /**
         * This flag indicates whether or not the file was
         * opened with write access.
         */
        bool writeAccess = false;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_FILE_POSIX_HPP */
