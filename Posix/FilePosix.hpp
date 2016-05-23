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

namespace SystemAbstractions {

    /**
     * This is the POSIX-specific state for the File class.
     */
    struct FileImpl {
        /**
         * @todo Needs documentation
         */
        FILE* handle;

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
