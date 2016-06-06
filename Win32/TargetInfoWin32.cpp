/**
 * @file TargetInfoWin32.cpp
 *
 * This module contains the Win32 specific part of the 
 * implementation of the TargetInfo functions.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "../TargetInfo.hpp"

namespace SystemAbstractions {

    std::string GetTargetArchitecture() {
#if defined(_WIN64)
        return "x64";
#else
        return "x86";
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
