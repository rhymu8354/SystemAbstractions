/**
 * @file StringExtensions.cpp
 *
 * This module contains the implementation of the StringExtensions functions.
 *
 * Copyright (c) 2014 by Richard Walters
 */

#include "StringExtensions.h"

#include <stdarg.h>
#include <vector>

namespace StringExtensions {

    std::string vsprintf(const char* format, va_list args) {
        va_list argsCopy;
        va_copy(argsCopy, args);
        const int required = vsnprintf(nullptr, 0, format, args);
        va_end(args);
        if (required < 0) {
            va_end(argsCopy);
            return "";
        }
        std::vector< char > buffer(required + 1);
        const int result = vsnprintf(&buffer[0], required + 1, format, argsCopy);
        va_end(argsCopy);
        if (result < 0) {
            return "";
        }
        return std::string(buffer.begin(), buffer.begin() + required);
    }

    std::string sprintf(const char* format, ...) {
        va_list args;
        va_start(args, format);
        return vsprintf(format, args);
    }

}
