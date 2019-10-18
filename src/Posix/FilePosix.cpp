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
#include <fcntl.h>
#include <pwd.h>
#include <regex>
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

    File::Impl::~Impl() noexcept = default;
    File::Impl::Impl(Impl&&) noexcept = default;
    File::Impl& File::Impl::operator=(Impl&&) = default;

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

    File::~File() noexcept {
        if (impl_ == nullptr) {
            return;
        }
        Close();
    }
    File::File(File&& other) noexcept = default;
    File& File::operator=(File&& other) noexcept = default;

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
        impl_->platform->handle = open(impl_->path.c_str(), O_RDONLY);
        impl_->platform->writeAccess = false;
        return (impl_->platform->handle >= 0);
    }

    void File::Close() {
        if (impl_->platform->handle < 0) {
            return;
        }
        (void)close(impl_->platform->handle);
        impl_->platform->handle = -1;
    }

    bool File::Create() {
        Close();
        impl_->platform->writeAccess = true;
        impl_->platform->handle = open(
            impl_->path.c_str(),
            O_RDWR | O_CREAT,
            S_IRUSR | S_IWUSR | S_IXUSR
        );
        auto isSuccessful = (impl_->platform->handle >= 0);
        if (!isSuccessful) {
            if (!Impl::CreatePath(impl_->path)) {
                return false;
            } else {
                impl_->platform->handle = open(
                    impl_->path.c_str(),
                    O_RDWR | O_CREAT,
                    S_IRUSR | S_IWUSR | S_IXUSR
                );
                isSuccessful = (impl_->platform->handle >= 0);
            }
        }
        return isSuccessful;
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
        if (impl_->platform->handle < 0) {
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

    bool File::IsAbsolutePath(const std::string& path) {
        static std::regex AbsolutePathRegex("[~/].*");
        return std::regex_match(path, AbsolutePathRegex);
    }

    std::string File::GetUserHomeDirectory() {
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

    std::string File::GetWorkingDirectory() {
        std::vector< char > workingDirectory(MAXPATHLEN);
        (void)getcwd(&workingDirectory[0], workingDirectory.size());
        return std::string(&workingDirectory[0]);
    }

    void File::SetWorkingDirectory(const std::string& workingDirectory) {
        (void)chdir(workingDirectory.c_str());
    }

    uint64_t File::GetSize() const {
        if (impl_->platform->handle < 0) {
            return 0;
        }
        const auto originalPosition = lseek(impl_->platform->handle, 0, SEEK_CUR);
        if (originalPosition == (off_t)-1) {
            return 0;
        }
        if (lseek(impl_->platform->handle, 0, SEEK_END) == (off_t)-1) {
            (void)lseek(impl_->platform->handle, originalPosition, SEEK_SET);
            return 0;
        }
        const auto endPosition = lseek(impl_->platform->handle, 0, SEEK_CUR);
        (void)lseek(impl_->platform->handle, originalPosition, SEEK_SET);
        if (endPosition == (off_t)-1) {
            return 0;
        }
        return (uint64_t)endPosition;
    }

    bool File::SetSize(uint64_t size) {
        const bool result = (ftruncate(impl_->platform->handle, (off_t)size) == 0);
        return result;
    }

    uint64_t File::GetPosition() const {
        if (impl_->platform->handle < 0) {
            return 0;
        }
        const auto position = lseek(impl_->platform->handle, 0, SEEK_CUR);
        if (position == (off_t)-1) {
            return 0;
        }
        return (uint64_t)position;
    }

    void File::SetPosition(uint64_t position) {
        if (impl_->platform->handle < 0) {
            return;
        }
        (void)lseek(impl_->platform->handle, (long)position, SEEK_SET);
    }

    size_t File::Peek(void* buffer, size_t numBytes) const {
        if (impl_->platform->handle < 0) {
            return 0;
        }
        const auto originalPosition = lseek(impl_->platform->handle, 0, SEEK_CUR);
        if (originalPosition == (off_t)-1) {
            return 0;
        }
        const auto readResult = read(impl_->platform->handle, buffer, numBytes);
        (void)lseek(impl_->platform->handle, originalPosition, SEEK_SET);
        return (
            (readResult < 0)
            ? (size_t)0
            : (size_t)readResult
        );
    }

    size_t File::Read(void* buffer, size_t numBytes) {
        if (impl_->platform->handle < 0) {
            return 0;
        }
        const auto readResult = read(impl_->platform->handle, buffer, numBytes);
        return (
            (readResult < 0)
            ? (size_t)0
            : (size_t)readResult
        );
    }

    size_t File::Write(const void* buffer, size_t numBytes) {
        if (impl_->platform->handle < 0) {
            return 0;
        }
        const auto amountWritten = write(impl_->platform->handle, buffer, numBytes);
        return (
            (amountWritten < 0)
            ? (size_t)0
            : (size_t)amountWritten
        );
    }

    std::shared_ptr< IFile > File::Clone() {
        auto clone = std::make_shared< File >(impl_->path);
        clone->impl_->platform->writeAccess = impl_->platform->writeAccess;
        if (impl_->platform->handle >= 0) {
            if (clone->impl_->platform->writeAccess) {
                clone->impl_->platform->handle = open(
                    impl_->path.c_str(),
                    O_RDWR | O_CREAT,
                    S_IRUSR | S_IWUSR | S_IXUSR
                );
            } else {
                clone->impl_->platform->handle = open(impl_->path.c_str(), O_RDONLY);
            }
            if (clone->impl_->platform->handle < 0) {
                return nullptr;
            }
        }
        return clone;
    }

}
