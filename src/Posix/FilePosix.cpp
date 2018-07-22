/**
 * @file OSFilePosix.cpp
 *
 * This module contains the Posix specific part of the
 * implementation of the SystemAbstractions::File class.
 *
 * Copyright (c) 2013-2016 by Richard Walters
 */

#include "../FileImpl.hpp"
#include "FilePosix.hpp"

#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <SystemAbstractions/File.hpp>
#include <unistd.h>
#include <vector>

namespace {

    /**
     * This is the maximum number of bytes to copy from one file to another
     * at a time.
     */
    static const size_t MAX_BLOCK_COPY_SIZE = 65536;

}

namespace SystemAbstractions {

    std::string GetUserHomeDirectoryPath() {
        auto suggestedBufferSize = sysconf(_SC_GETPW_R_SIZE_MAX);
        const size_t bufferSize = ((suggestedBufferSize < 0) ? 65536 : suggestedBufferSize);
        std::vector< char > buffer(bufferSize);
        struct passwd pwd;
        struct passwd* resultEntry;
        (void)getpwuid_r(getuid(), &pwd, &buffer[0], bufferSize, &resultEntry);
        if (resultEntry == NULL) {
            return "";
        } else {
            return pwd.pw_dir;
        }
    }

    File::Impl::~Impl() = default;
    File::Impl::Impl(Impl&&) = default;
    File::Impl& File::Impl::operator =(Impl&&) = default;

    File::Impl::Impl()
        : platform(new Platform())
    {
    }

    bool File::Impl::CreatePath(std::string path) {
        const size_t delimiter = path.find_last_of("/\\");
        if (delimiter == std::string::npos) {
            return false;
        }
        const std::string oneLevelUp(path.substr(0, delimiter));
        if (mkdir(oneLevelUp.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) == 0) {
            return true;
        }
        if (errno == EEXIST) {
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

    File::~File() {
        Close();
    }
    File::File(File&& other) = default;
    File& File::operator=(File&& other) = default;

    File::File(std::string path)
        : impl_(new Impl())
    {
        impl_->path = path;
    }

    bool File::IsExisting() {
        return (access(impl_->path.c_str(), 0) == 0);
    }

    bool File::IsDirectory() {
        struct stat s;
        if (
            (stat(impl_->path.c_str(), &s) == 0)
            && (S_ISDIR(s.st_mode))
        ) {
            return s.st_mtime;
        } else {
            return false;
        }
    }

    bool File::Open() {
        Close();
        impl_->platform->handle = fopen(impl_->path.c_str(), "rb");
        return (impl_->platform->handle != NULL);
    }

    void File::Close() {
        if (impl_->platform->handle == NULL) {
            return;
        }
        (void)fclose(impl_->platform->handle);
        impl_->platform->handle = NULL;
    }

    bool File::Create() {
        Close();
        impl_->platform->handle = fopen(impl_->path.c_str(), "w+b");
        if (impl_->platform->handle == NULL) {
            if (!Impl::CreatePath(impl_->path)) {
                return false;
            } else {
                impl_->platform->handle = fopen(impl_->path.c_str(), "w+b");
            }
        }
        return (impl_->platform->handle != NULL);
    }

    void File::Destroy() {
        Close();
        (void)remove(impl_->path.c_str());
    }

    bool File::Move(const std::string& newPath) {
        if (rename(impl_->path.c_str(), newPath.c_str()) != 0) {
            return false;
        }
        impl_->path = newPath;
        return true;
    }

    bool File::Copy(const std::string& destination) {
        if (impl_->platform->handle == NULL) {
            if (!Open()) {
                return false;
            }
        } else {
            SetPosition(0);
        }
        File newFile(destination);
        if (!newFile.Create()) {
            return false;
        }
        IFile::Buffer buffer(MAX_BLOCK_COPY_SIZE);
        for (;;) {
            const size_t amt = Read(buffer);
            if (amt == 0) {
                break;
            }
            if (newFile.Write(buffer, amt) != amt) {
                return false;
            }
        }
        return true;
    }

    time_t File::GetLastModifiedTime() const {
        struct stat s;
        if (stat(impl_->path.c_str(), &s) == 0) {
            return s.st_mtime;
        } else {
            return 0;
        }
    }

    void File::ListDirectory(const std::string& directory, std::vector< std::string >& list) {
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
                std::string name(entry.d_name);
                if (
                    (name == ".")
                    || (name == "..")
                ) {
                    continue;
                }
                std::string filePath(directoryWithSeparator);
                filePath += name;
                list.push_back(filePath);
            }
            (void)closedir(dir);
        }
    }

    bool File::DeleteDirectory(const std::string& directory) {
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
                    if (!DeleteDirectory(filePath.c_str())) {
                        return false;
                    }
                } else {
                    if (unlink(filePath.c_str()) != 0) {
                        return false;
                    }
                }
            }
            (void)closedir(dir);
            return (rmdir(directory.c_str()) == 0);
        }
        return true;
    }

    bool File::CopyDirectory(
        const std::string& existingDirectory,
        const std::string& newDirectory
    ) {
        std::string existingDirectoryWithSeparator(existingDirectory);
        if (
            (existingDirectoryWithSeparator.length() > 0)
            && (existingDirectoryWithSeparator[existingDirectoryWithSeparator.length() - 1] != '/')
        ) {
            existingDirectoryWithSeparator += '/';
        }
        std::string newDirectoryWithSeparator(newDirectory);
        if (
            (newDirectoryWithSeparator.length() > 0)
            && (newDirectoryWithSeparator[newDirectoryWithSeparator.length() - 1] != '/')
        ) {
            newDirectoryWithSeparator += '/';
        }
        if (!Impl::CreatePath(newDirectoryWithSeparator)) {
            return false;
        }
        DIR* dir = opendir(existingDirectory.c_str());
        if (dir != NULL) {
            struct dirent entry;
            struct dirent* entryBack;
            std::vector< char > link(PATH_MAX);
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
                std::string filePath(existingDirectoryWithSeparator);
                filePath += entry.d_name;
                std::string newFilePath(newDirectoryWithSeparator);
                newFilePath += entry.d_name;
                if (entry.d_type == DT_DIR) {
                    if (!CopyDirectory(filePath, newFilePath)) {
                        return false;
                    }
                } else if (entry.d_type == DT_LNK) {
                    if (readlink(filePath.c_str(), &link[0], link.size()) < 0) {
                        return false;
                    }
                    if (symlink(&link[0], newFilePath.c_str()) < 0) {
                        return false;
                    }
                } else {
                    File file(filePath);
                    if (!file.Copy(newFilePath)) {
                        return false;
                    }
                }
            }
            (void)closedir(dir);
        }
        return true;
    }

    std::vector< std::string > File::GetDirectoryRoots() {
        return {"/"};
    }

    uint64_t File::GetSize() const {
        if (impl_->platform->handle == NULL) {
            return 0;
        }
        const long originalPosition = ftell(impl_->platform->handle);
        if (originalPosition == EOF) {
            return 0;
        }
        if (fseek(impl_->platform->handle, 0, SEEK_END) == EOF) {
            (void)fseek(impl_->platform->handle, originalPosition, SEEK_SET);
            return 0;
        }
        const long endPosition = ftell(impl_->platform->handle);
        (void)fseek(impl_->platform->handle, originalPosition, SEEK_SET);
        if (endPosition == EOF) {
            return 0;
        }
        return (uint64_t)endPosition;
    }

    bool File::SetSize(uint64_t size) {
        return (ftruncate(fileno(impl_->platform->handle), (off_t)size) == 0);
    }

    uint64_t File::GetPosition() const {
        if (impl_->platform->handle == NULL) {
            return 0;
        }
        const long position = ftell(impl_->platform->handle);
        if (position == EOF) {
            return 0;
        }
        return (uint64_t)position;
    }

    void File::SetPosition(uint64_t position) {
        if (impl_->platform->handle == NULL) {
            return;
        }
        (void)fseek(impl_->platform->handle, (long)position, SEEK_SET);
    }

    size_t File::Peek(void* buffer, size_t numBytes) const {
        if (impl_->platform->handle == NULL) {
            return 0;
        }
        const long originalPosition = ftell(impl_->platform->handle);
        if (originalPosition == EOF) {
            return 0;
        }
        const size_t readResult = fread(buffer, 1, numBytes, impl_->platform->handle);
        (void)fseek(impl_->platform->handle, originalPosition, SEEK_SET);
        return readResult;
    }

    size_t File::Read(void* buffer, size_t numBytes) {
        if (impl_->platform->handle == NULL) {
            return 0;
        }
        return fread(buffer, 1, numBytes, impl_->platform->handle);
    }

    size_t File::Write(const void* buffer, size_t numBytes) {
        if (impl_->platform->handle == NULL) {
            return 0;
        }
        return fwrite(buffer, 1, numBytes, impl_->platform->handle);
    }

    std::shared_ptr< IFile > File::Clone() {
        auto clone = std::make_shared< File >(impl_->path);
        if (impl_->platform->handle != NULL) {
            int cloneHandle = dup(fileno(impl_->platform->handle));
            if (cloneHandle < 0) {
                return nullptr;
            }
            clone->impl_->platform->handle = fdopen(cloneHandle, "r+b");
            if (clone->impl_->platform->handle == NULL) {
                (void)close(cloneHandle);
                return nullptr;
            }
        }
        return clone;
    }

}
