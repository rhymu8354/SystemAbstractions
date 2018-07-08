#ifndef SYSTEM_ABSTRACTIONS_TARGET_INFO_HPP
#define SYSTEM_ABSTRACTIONS_TARGET_INFO_HPP

/**
 * @file TargetInfo.hpp
 *
 * This module declares functions which obtain information about the
 * target on which the program is running.
 *
 * Copyright (c) 2014-2016 by Richard Walters
 */

#include <string>

namespace SystemAbstractions {

    /**
     * @todo Needs documentation
     */
    std::string GetTargetArchitecture();

    /**
     * @todo Needs documentation
     */
    std::string GetTargetVariant();

}

#endif /* SYSTEM_ABSTRACTIONS_TARGET_INFO_HPP */
