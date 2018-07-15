#ifndef SYSTEM_ABSTRACTIONS_TARGET_INFO_HPP
#define SYSTEM_ABSTRACTIONS_TARGET_INFO_HPP

/**
 * @file TargetInfo.hpp
 *
 * This module declares functions which obtain information about the
 * target on which the program is running.
 *
 * © 2016-2018 by Richard Walters
 */

#include <string>

namespace SystemAbstractions {

    /**
     * This function returns an identifier corresponding to the machine
     * architecture for which the currently running program was built.
     *
     * @return
     *     An identifier corresponding to the machine architecture for
     *     which the currently running program was built is returned.
     */
    std::string GetTargetArchitecture();

    /**
     * This function returns an identifier corresponding to the
     * build variant that was selected to build the currently running
     * program.
     *
     * @return
     *     An identifier corresponding to the build variant that was
     *     selected to build the currently running program.
     */
    std::string GetTargetVariant();

}

#endif /* SYSTEM_ABSTRACTIONS_TARGET_INFO_HPP */
