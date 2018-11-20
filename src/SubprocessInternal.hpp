#ifndef SYSTEM_ABSTRACTIONS_SUBPROCESS_INTERNAL_HPP
#define SYSTEM_ABSTRACTIONS_SUBPROCESS_INTERNAL_HPP

/**
 * @file SubprocessInternal.hpp
 *
 * This module declares functions used internally by the Subprocess module
 * of the SystemAbstractions library.
 *
 * © 2018 by Richard Walters
 */

namespace SystemAbstractions {

    /**
     * This function closes all file handles currently open in the process,
     * except for the given one.
     *
     * @param[in] keepOpen
     *     This is the file handle to keep open.
     */
    void CloseAllFilesExcept(int keepOpen);
}

#endif /* SYSTEM_ABSTRACTIONS_SUBPROCESS_INTERNAL_HPP */
