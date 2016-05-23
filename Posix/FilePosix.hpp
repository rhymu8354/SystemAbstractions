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


namespace SystemAbstractions {

    /**
     * This is the POSIX-specific state for the File class.
     */
    struct OSFileImpl {
        /**
         * @todo Needs documentation
         */
        FILE* handle;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_FILE_POSIX_HPP */
