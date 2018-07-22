/**
 * @file TargetInfoLinux.cpp
 *
 * This module contains the Linux specific part of the
 * implementation of the TargetInfo functions.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include <SystemAbstractions/TargetInfo.hpp>

namespace SystemAbstractions {

    std::string GetTargetArchitecture() {
#if defined(__arm__)
        return "Linux-ARM";
#else
        return "Linux";
#endif
    }

    std::string GetTargetVariant() {
#if defined(_DEBUG)
        return "Debug";
#else
        return "Release";
#endif
    }

}
