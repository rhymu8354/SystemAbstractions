/**
 * @file OSFileMach.cpp
 *
 * This module contains the Mac (e.g. Mac OS X) specific part of the
 * implementation of the Files::OSFile class.
 *
 * Copyright (c) 2013-2016 by Richard Walters
 */

#include "../OSFile.hpp"

#include <CoreFoundation/CoreFoundation.h>
#include <dirent.h>
#include <mach-o/dyld.h>
#include <errno.h>
#include <pwd.h>
#include <StringExtensions/StringExtensions.hpp>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace Files {

    /**
     * This is the Mach-specific state for the OSFile class.
     */
    struct OSFileImpl {
        FILE* handle;
    };

    OSFile::OSFile(std::string path)
        : _path(path)
        , _impl(new OSFileImpl())
    {
    }

    OSFile::~OSFile() {
        Close();
    }

    bool OSFile::IsExisting() {
        return (access(_path.c_str(), 0) == 0);
    }

    bool OSFile::Open() {
        Close();
        _impl->handle = fopen(_path.c_str(), "r+b");
        return (_impl->handle != NULL);
    }

    void OSFile::Close() {
        if (_impl->handle == NULL) {
            return;
        }
        (void)fclose(_impl->handle);
        _impl->handle = NULL;
    }

    bool OSFile::Create() {
        Close();
        _impl->handle = fopen(_path.c_str(), "w+b");
        if (_impl->handle == NULL) {
            if (!CreatePath(_path)) {
                return false;
            } else {
                _impl->handle = fopen(_path.c_str(), "w+b");
            }
        }
        return (_impl->handle != NULL);
    }

    void OSFile::Destroy() {
        Close();
        (void)remove(_path.c_str());
    }

    std::string OSFile::GetExeParentDirectory() {
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

    std::string OSFile::GetResourceFilePath(const std::string& name) {
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
        DIR* dir = opendir(directory.c_str());
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
        DIR* dir = opendir(directory.c_str());
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
            (void)rmdir(directory.c_str());
        }
    }

    uint64_t OSFile::GetSize() const {
        if (_impl->handle == NULL) {
            return 0;
        }
        const long originalPosition = ftell(_impl->handle);
        if (originalPosition == EOF) {
            return 0;
        }
        if (fseek(_impl->handle, 0, SEEK_END) == EOF) {
            (void)fseek(_impl->handle, originalPosition, SEEK_SET);
            return 0;
        }
        const long endPosition = ftell(_impl->handle);
        (void)fseek(_impl->handle, originalPosition, SEEK_SET);
        if (endPosition == EOF) {
            return 0;
        }
        return (uint64_t)endPosition;
    }

    bool OSFile::SetSize(uint64_t size) {
        return (ftruncate(fileno(_impl->handle), (off_t)size) == 0);
    }

    uint64_t OSFile::GetPosition() const {
        if (_impl->handle == NULL) {
            return 0;
        }
        const long position = ftell(_impl->handle);
        if (position == EOF) {
            return 0;
        }
        return (uint64_t)position;
    }

    void OSFile::SetPosition(uint64_t position) {
        if (_impl->handle == NULL) {
            return;
        }
        (void)fseek(_impl->handle, (long)position, SEEK_SET);
    }

    size_t OSFile::Peek(void* buffer, size_t numBytes) const {
        if (_impl->handle == NULL) {
            return 0;
        }
        const long originalPosition = ftell(_impl->handle);
        if (originalPosition == EOF) {
            return 0;
        }
        const size_t readResult = fread(buffer, 1, numBytes, _impl->handle);
        (void)fseek(_impl->handle, originalPosition, SEEK_SET);
        return readResult;
    }

    size_t OSFile::Read(void* buffer, size_t numBytes) {
        if (_impl->handle == NULL) {
            return 0;
        }
        return fread(buffer, 1, numBytes, _impl->handle);
    }

    size_t OSFile::Write(const void* buffer, size_t numBytes) {
        if (_impl->handle == NULL) {
            return 0;
        }
        return fwrite(buffer, 1, numBytes, _impl->handle);
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
