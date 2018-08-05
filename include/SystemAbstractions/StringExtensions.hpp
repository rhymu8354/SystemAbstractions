#ifndef SYSTEM_ABSTRACTIONS_STRING_EXTENSIONS_HPP
#define SYSTEM_ABSTRACTIONS_STRING_EXTENSIONS_HPP

/**
 * @file StringExtensions.hpp
 *
 * This module declares functions which extend the string library.
 *
 * Copyright © 2014-2018 by Richard Walters
 */

#include <set>
#include <stdarg.h>
#include <string>
#include <vector>

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

    /**
     * @todo Needs documentation
     */
    std::string ParseElement(const std::string& s, size_t begin, size_t end);

    /**
     * @todo Needs documentation
     */
    std::string Escape(const std::string& s, char escapeCharacter, const std::set< char >& charactersToEscape);

    /**
     * @todo Needs documentation
     */
    std::string Unescape(const std::string& s, char escapeCharacter);

    /**
     * @todo Needs documentation
     */
    std::vector< std::string > Split(
        const std::string& s,
        char d
    );

    /**
     * This function joins together the given sequence of smaller strings
     * into one bigger string, with each piece separated by the given
     * delimiter string.
     *
     * @param[in] v
     *     This is the sequence of smaller strings to join together.
     *
     * @param[in] d
     *     This is the delimiter string to put between each piece.
     *
     * @return
     *     A single larger string formed by joining together the given
     *     pieces separated by delimiters is returned.
     */
    std::string Join(
        const std::vector< std::string >& v,
        const std::string& d
    );

    /**
     * This function takes a string and swaps all upper-case characters
     * with their lower-case equivalents, returning the result.
     *
     * @param[in] inString
     *     This is the string to be normalized.
     *
     * @return
     *     The normalized string is returned.  All upper-case characters
     *     are replaced with their lower-case equivalents.
     */
    std::string ToLower(const std::string& inString);

}

#endif /* SYSTEM_ABSTRACTIONS_STRING_EXTENSIONS_HPP */
