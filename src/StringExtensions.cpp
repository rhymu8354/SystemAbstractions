/**
 * @file StringExtensions.cpp
 *
 * This module contains the implementation of fucntions
 * which extend the string library.
 *
 * Copyright © 2014-2018 by Richard Walters
 */

#include <SystemAbstractions/StringExtensions.hpp>

#include <stdarg.h>
#include <stdlib.h>
#include <sstream>
#include <vector>

namespace SystemAbstractions {

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

    std::string wcstombs(const std::wstring& src) {
        std::vector< char > buffer(src.length() * MB_CUR_MAX + 1);
        (void)::wcstombs(&buffer[0], src.c_str(), buffer.size());
        return std::string(&buffer[0]);
    }

    std::string Trim(const std::string& s) {
        size_t i = 0;
        while (
            (i < s.length())
            && (s[i] <= 32)
        ) {
            ++i;
        }
        size_t j = s.length();
        while (
            (j > 0)
            && (s[j - 1] <= 32)
        ) {
            --j;
        }
        return s.substr(i, j - i);
    }

    std::string Indent(std::string linesIn, size_t spaces) {
        std::string linesOut;
        while (!linesIn.empty()) {
            std::string line;
            auto delimiter = linesIn.find("\r\n");
            if (delimiter == std::string::npos) {
                line = linesIn;
                linesIn.clear();
            } else {
                line = linesIn.substr(0, delimiter + 2);
                linesIn = linesIn.substr(delimiter + 2);
            }
            if (!linesOut.empty()) {
                linesOut.append(spaces, ' ');
            }
            linesOut += line;
        }
        return linesOut;
    }

    std::string ParseComponent(const std::string& s, size_t begin, size_t end) {
        bool inString = false;
        int level = 1;
        size_t j = begin;
        bool escape = false;
        while (
            (j < end)
            && (level > 0)
        ) {
            if (inString) {
                if (escape) {
                    escape = false;
                } else {
                    if (s[j] == '\\') {
                        escape = true;
                    } else if (s[j] == '"') {
                        inString = false;
                    }
                }
            } else {
                if (
                    (level == 1)
                    && (s[j] == ',')
                ) {
                    break;
                } else if (s[j] == '"') {
                    inString = true;
                } else if (
                    (s[j] == '[')
                    || (s[j] == '{')
                    || (s[j] == '(')
                    || (s[j] == '<')
                ) {
                    ++level;
                } else if (
                    (s[j] == ']')
                    || (s[j] == '}')
                    || (s[j] == ')')
                    || (s[j] == '>')
                ) {
                    --level;
                }
            }
            ++j;
        }
        return s.substr(begin, j - begin);
    }

    std::string Escape(const std::string& s, char escapeCharacter, const std::set< char >& charactersToEscape) {
        std::string output;
        for (size_t i = 0; i < s.length(); ++i) {
            if (charactersToEscape.find(s[i]) != charactersToEscape.end()) {
                output += escapeCharacter;
            }
            output += s[i];
        }
        return output;
    }

    std::string Unescape(const std::string& s, char escapeCharacter) {
        std::string output;
        bool escape = false;
        for (size_t i = 0; i < s.length(); ++i) {
            if (
                !escape
                && (s[i] == escapeCharacter)
            ) {
                escape = true;
            } else {
                output += s[i];
                escape = false;
            }
        }
        return output;
    }

    std::vector< std::string > Split(
        const std::string& s,
        char d
    ) {
        std::vector< std::string > values;
        auto remainder = Trim(s);
        while (!remainder.empty()) {
            auto delimiter = remainder.find_first_of(d);
            if (delimiter == std::string::npos) {
                values.push_back(remainder);
                remainder.clear();
            } else {
                values.push_back(Trim(remainder.substr(0, delimiter)));
                remainder = Trim(remainder.substr(delimiter + 1));
            }
        }
        return values;
    }

    std::string Join(
        const std::vector< std::string >& v,
        const std::string& d
    ) {
        std::ostringstream builder;
        bool first = true;
        for (const auto& piece: v) {
            if (first) {
                first = false;
            } else {
                builder << d;
            }
            builder << piece;
        }
        return builder.str();
    }

    std::string ToLower(const std::string& inString) {
        std::string outString;
        outString.reserve(inString.size());
        for (char c: inString) {
            outString.push_back(tolower(c));
        }
        return outString;
    }

    ToIntegerResult ToInteger(
        const std::string& numberString,
        intmax_t& number
    ) {
        size_t index = 0;
        size_t state = 0;
        bool negative = false;
        intmax_t value = 0;
        while (index < numberString.size()) {
            switch (state) {
                case 0: { // [ minus ]
                    if (numberString[index] == '-') {
                        negative = true;
                        ++index;
                    }
                    state = 1;
                } break;

                case 1: { // zero / 1-9
                    if (numberString[index] == '0') {
                        state = 2;
                    } else if (
                        (numberString[index] >= '1')
                        && (numberString[index] <= '9')
                    ) {
                        state = 3;
                        value = (decltype(value))(numberString[index] - '0');
                        value = (value * (negative ? -1 : 1));
                    } else {
                        return ToIntegerResult::NotANumber;
                    }
                    ++index;
                } break;

                case 2: { // extra junk!
                    return ToIntegerResult::NotANumber;
                } break;

                case 3: { // *DIGIT
                    if (
                        (numberString[index] >= '0')
                        && (numberString[index] <= '9')
                    ) {
                        const auto digit = (decltype(value))(numberString[index] - '0');
                        if (negative) {
                            if ((std::numeric_limits< decltype(value) >::lowest() + digit) / 10 > value) {
                                return ToIntegerResult::Overflow;
                            }
                        } else {
                            if ((std::numeric_limits< decltype(value) >::max() - digit) / 10 < value) {
                                return ToIntegerResult::Overflow;
                            }
                        }
                        value *= 10;
                        if (negative) {
                            value -= digit;
                        } else {
                            value += digit;
                        }
                        ++index;
                    } else {
                        return ToIntegerResult::NotANumber;
                    }
                } break;
            }
        }
        if (state >= 2) {
            number = value;
            return ToIntegerResult::Success;
        } else {
            return ToIntegerResult::NotANumber;
        }
    }

}
