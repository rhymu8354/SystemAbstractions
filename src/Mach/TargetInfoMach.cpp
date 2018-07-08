/**
 * @file TargetInfoMach.cpp
 *
 * This module contains the Mach specific part of the 
 * implementation of the TargetInfo functions.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "../TargetInfo.hpp"

#include <TargetConditionals.h>

namespace SystemAbstractions {

    std::string GetTargetArchitecture() {
#if defined(TARGET_OS_MAC)
        return "Mach";
#elif defined(TARGET_OS_IPHONE)
        reutrn "iOS";
#else
        return "Apple";
#endif
    }

    std::string GetTargetVariant() {
#if defined(_DEBUG) || (DEBUG != 0)
        return "Debug";
#else
        return "Release";
#endif
    }

}
