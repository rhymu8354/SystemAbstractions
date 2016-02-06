#ifndef STRING_EXTENSIONS_HPP
#define STRING_EXTENSIONS_HPP

/**
 * @file StringExtensions.hpp
 *
 * This module declares the StringExtensions functions.
 *
 * Copyright (c) 2014-2016 by Richard Walters
 */

#include <stdarg.h>
#include <string>

namespace StringExtensions {

    /**
     * This function is like the vsprintf funtion in the standard C
     * library, except that it constructs the string dynamically and
     * returns it as a std::string value.
     */
    std::string vsprintf(const char* format, va_list args);

    /**
     * This function is like the sprintf funtion in the standard C
     * library, except that it constructs the string dynamically and
     * returns it as a std::string value.
     */
    std::string sprintf(const char* format, ...);

    /**
     * This method converts a given string from wide (UNICODE) to
     * narrow (multibyte) format, using the currently set locale.
     *
     * This method has the same semantics as the C function "wcstombs".
     *
     * @param[in] src
     *     This is the string to convert.
     *
     * @return
     *     The converted string is returned.
     */
    std::string wcstombs(const std::wstring& src);

}

#endif /* STRING_EXTENSIONS_HPP */
