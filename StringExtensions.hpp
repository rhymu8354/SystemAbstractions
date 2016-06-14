#ifndef SYSTEM_ABSTRACTIONS_STRING_EXTENSIONS_HPP
#define SYSTEM_ABSTRACTIONS_STRING_EXTENSIONS_HPP

/**
 * @file StringExtensions.hpp
 *
 * This module declares functions which extend the string library.
 *
 * Copyright (c) 2014-2016 by Richard Walters
 */

#include <stdarg.h>
#include <string>

namespace SystemAbstractions {

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

    /**
     * This method makes a copy of a string and removes any whitespace
     * from the front and back of the copy.
     *
     * @param[in] s
     *     This is the string to trim.
     *
     * @return
     *     The trimmed string is returned.
     */
    std::string Trim(const std::string& s);

    /**
     * @todo Needs documentation
     */
    std::string Indent(std::string linesIn, size_t spaces);

}

#endif /* SYSTEM_ABSTRACTIONS_STRING_EXTENSIONS_HPP */
