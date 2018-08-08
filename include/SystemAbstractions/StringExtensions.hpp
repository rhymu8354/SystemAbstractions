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
     * This function breaks up the given string into lines,
     * according to any CR-LF end-of-line character sequences found
     * in the string, indents all lines except the first,
     * and then concatenates the lines back together, returning
     * them as a single string.
     *
     * @param[in] linesIn
     *     This is the string containing the lines to indent.
     *
     * @param[in] spaces
     *     This is the number of spaces to indent each line but the first.
     *
     * @return
     *     The indented text is returned as a single string.
     */
    std::string Indent(std::string linesIn, size_t spaces);

    /**
     * This function returns a substring of the given string that
     * contains the next fully delimited "component", such as string,
     * array, object, etc., given the following recognized delimiter pairs:
     * - double quotes
     * - [ ]
     * - { }
     * - < >
     * - ( )
     *
     * Escaped delimiter characters (i.e. "\{") are not considered delimiters.
     *
     * Any comma (',') or "outer-most" delimiter encountered while scanning
     * the string is considered to terminate the component.
     *
     * @param[in] s
     *     This is the string from which to extract the next
     *     delimited component.
     *
     * @param[in] begin
     *     This is the starting position from which to scan
     *     the next component.
     *
     * @param[in] end
     *     This is the limit to which the string will be scanned
     *     to determine the next component.
     */
    std::string ParseComponent(const std::string& s, size_t begin, size_t end);

    /**
     * This function returns a copy of the given input string, modified
     * so that every character in the given "charactersToEscape" that is
     * found in the input string is prefixed by the given "escapeCharacter".
     *
     * @param[in] s
     *     This is the input string.
     *
     * @param[in] escapeCharacter
     *     This is the character to put in front of every character
     *     in the input string that is a member of the
     *     "charactersToEscape" set.
     *
     * @param[in] charactersToEscape
     *     These are the characters that should be escaped in the input.
     *
     * @return
     *     A copy of the input string is returned, where every character
     *     in the given "charactersToEscape" that is found in the input
     *     string is prefixed by the given "escapeCharacter".
     */
    std::string Escape(const std::string& s, char escapeCharacter, const std::set< char >& charactersToEscape);

    /**
     * This function removes the given escapeCharacter from the given
     * input string, returning the result.
     *
     * @param[in] s
     *     This is the string from which to remove all escape characters.
     *
     * @param[in] escapeCharacter
     *     This is the character to remove from the given input string.
     *
     * @return
     *     A copy of the given input string, with all instances of the
     *     given escape character removed, is returned.
     */
    std::string Unescape(const std::string& s, char escapeCharacter);

    /**
     * This function breaks the given string at each instance of the
     * given delimiter, returning the pieces as a collection of substrings.
     * The delimiter characters are removed.
     *
     * @param[in] s
     *     This is the string to split.
     *
     * @param[in] d
     *     This is the delimiter character at which to split the string.
     *
     * @return
     *     The collection of substrings that result from breaking the given
     *     string at each delimiter character is returned.
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
