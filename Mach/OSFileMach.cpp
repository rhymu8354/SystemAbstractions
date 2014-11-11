/**
 * @file OSFileMach.cpp
 *
 * This module contains the Mac (e.g. Mac OS X) specific part of the
 * implementation of the Files::OSFile class.
 *
 * Copyright (c) 2013-2014 by Richard Walters
 */

#include "../OSFile.h"

#include <dirent.h>
#include <mach-o/dyld.h>
#include <errno.h>
#include <StringExtensions/StringExtensions.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace Files {

    bool OSFile::IsExisting() {
        return (access(_path.c_str(), 0) == 0);
    }

    std::string OSFile::GetExeDirectory() {
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

    std::string OSFile::GetResourcesDirectory() {
        return StringExtensions::sprintf("%s/../Resources", GetExeDirectory().c_str());
    }

    std::string OSFile::GetUserSavedGamesDirectory(const std::string& nameKey) {
        struct passwd* pw = getpwuid(getuid());
        return StringExtensions::sprintf("%s/Library/Application Support/%s/Saved Games", pw->pw_dir, nameKey.c_str());
    }

    void OSFile::ListDirectory(const std::string& directory, std::vector< std::string >& list) {
        std::string directoryWithSeparator(directory);
        if (
            (directoryWithSeparator.length() > 0)
            && (directoryWithSeparator[directoryWithSeparator.length() - 1] != '/')
            ) {
            directoryWithSeparator += '/';
        }
        list.clear();
        DIR* dir = opendir(directory);
        if (dir != NULL) {
            struct dirent entry;
            struct dirent* entryBack;
            while (true) {
                if (readdir_r(dir, &entry, &entryBack)) {
                    break;
                }
                if (entryBack == NULL) {
                    break;
                }
                std::string filePath(directoryWithSeparator);
                filePath += entry.d_name;
                list.push_back(filePath);
            }
            (void)closedir(dir);
        }
    }

    void OSFile::DeleteDirectory(const std::string& directory) {
        std::string directoryWithSeparator(directory);
        if (
            (directoryWithSeparator.length() > 0)
            && (directoryWithSeparator[directoryWithSeparator.length() - 1] != '/')
            ) {
            directoryWithSeparator += '/';
        }
        DIR* dir = opendir(directory);
        if (dir != NULL) {
            struct dirent entry;
            struct dirent* entryBack;
            while (true) {
                if (readdir_r(dir, &entry, &entryBack)) {
                    break;
                }
                if (entryBack == NULL) {
                    break;
                }
                std::string name(entry.d_name);
                if (
                    (name == ".")
                    || (name == "..")
                ) {
                    continue;
                }
                std::string filePath(directoryWithSeparator);
                filePath += entry.d_name;
                if (entry.d_type == DT_DIR) {
                    DeleteDirectory(filePath.c_str());
                } else {
                    (void)unlink(filePath.c_str());
                }
            }
            (void)closedir(dir);
            (void)rmdir(directory);
        }
    }

    bool OSFile::SetSize(uint64_t size) {
        return (ftruncate(fileno(_handle), (off_t)size) == 0);
    }

    bool OSFile::CreatePath(std::string path) {
        const size_t delimiter = path.find_last_of("/\\");
        if (delimiter == std::string::npos) {
            return false;
        }
        const std::string oneLevelUp(path.substr(0, delimiter));
        if (mkdir(oneLevelUp.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) == 0) {
            return true;
        }
        if (errno != ENOENT) {
            return false;
        }
        if (!CreatePath(oneLevelUp)) {
            return false;
        }
        if (mkdir(oneLevelUp.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) != 0) {
            return false;
        }
        return true;
    }

}
