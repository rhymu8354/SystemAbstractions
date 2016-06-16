/**
 * @file StringExtensions.cpp
 *
 * This module contains the implementation of fucntions
 * which extend the string library.
 *
 * Copyright (c) 2014 by Richard Walters
 */

#include "StringExtensions.hpp"

#include <stdarg.h>
#include <stdlib.h>
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

    std::string ParseElement(const std::string&s, size_t begin, size_t end) {
        bool inString = false;
        int level = 1;
        size_t j = begin;
        while (
            (j < end)
            && (level > 0)
        ) {
            if (inString) {
                if (s[j] == '"') {
                    inString = false;
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

    std::vector< std::string > Split(const std::string& s) {
        size_t i = 1;
        std::vector< std::string > values;
        const size_t end = s.length() - 1;
        while (i < s.length() - 1) {
            const std::string element = ParseElement(s, i, end);
            const std::string value(Trim(element));
            if (!value.empty()) {
                values.emplace_back(std::move(value));
            }
            const size_t j = i + element.length();
            if (s[j] == ',') {
                i = j + 1;
            } else {
                i = j;
            }
        }
        return values;
    }

}
