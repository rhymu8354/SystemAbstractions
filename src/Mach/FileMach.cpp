/**
 * @file FileMach.cpp
 *
 * This module contains the Mac (e.g. Mac OS X) specific part of the
 * implementation of the SystemAbstractions::File class.
 *
 * Copyright (c) 2013-2016 by Richard Walters
 */

#include "../Posix/FilePosix.hpp"

#include <CoreFoundation/CoreFoundation.h>
#include <dirent.h>
#include <mach-o/dyld.h>
#include <errno.h>
#include <pwd.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <StringExtensions/StringExtensions.hpp>
#include <sys/stat.h>
#include <sys/types.h>
#include <SystemAbstractions/File.hpp>
#include <unistd.h>
#include <vector>

namespace SystemAbstractions {

    std::string File::GetExeImagePath() {
        // Get the path to the executable.
        std::vector< char > buffer(PATH_MAX);
        uint32_t length = 0;
        if (_NSGetExecutablePath(&buffer[0], &length) < 0) {
            buffer.resize(length);
            (void)_NSGetExecutablePath(&buffer[0], &length);
        }
        const std::string pathWithLinks(&buffer[0]);

        // The returned path might include symbolic links,
        // so use realpath to reduce the path to an absolute path.
        (void)realpath(pathWithLinks.c_str(), &buffer[0]);
        return std::string(&buffer[0]);
    }

    std::string File::GetExeParentDirectory() {
        // Get the path to the executable.
        std::vector< char > buffer(PATH_MAX);
        uint32_t length = 0;
        if (_NSGetExecutablePath(&buffer[0], &length) < 0) {
            buffer.resize(length);
            (void)_NSGetExecutablePath(&buffer[0], &length);
        }
        const std::string pathWithLinks(&buffer[0]);

        // The returned path might include symbolic links,
        // so use realpath to reduce the path to an absolute path.
        (void)realpath(pathWithLinks.c_str(), &buffer[0]);
        length = static_cast< uint32_t >(strlen(&buffer[0]));
        while (--length > 0) {
            if (buffer[length] == '/') {
                break;
            }
        }
        if (length == 0) {
            ++length;
        }
        buffer[length] = '\0';
        return std::string(&buffer[0]);
    }

    std::string File::GetResourceFilePath(const std::string& name) {
        CFURLRef appUrlRef;
        CFStringRef nameCfString = CFStringCreateWithCString(NULL, name.c_str(), kCFStringEncodingUTF8);
        appUrlRef = CFBundleCopyResourceURL(CFBundleGetMainBundle(), nameCfString, NULL, NULL);
        if (appUrlRef == nullptr) {
            return "";
        } else {
            CFStringRef filePathRef = CFURLCopyPath(appUrlRef);
            return CFStringGetCStringPtr(filePathRef, kCFStringEncodingUTF8);
        }
    }

    std::string File::GetLocalPerUserConfigDirectory(const std::string& nameKey) {
        return StringExtensions::sprintf("%s/Library/Application Support/%s", GetUserHomeDirectory().c_str(), nameKey.c_str());
    }

    std::string File::GetUserSavedGamesDirectory(const std::string& nameKey) {
        return StringExtensions::sprintf("%s/Library/Application Support/%s/Saved Games", GetUserHomeDirectory().c_str(), nameKey.c_str());
    }

}
