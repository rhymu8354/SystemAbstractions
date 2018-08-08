/**
 * @file FileWin32.cpp
 *
 * This module contains the Win32 specific part of the 
 * implementation of the SystemAbstractions::File class.
 *
 * © 2013-2018 by Richard Walters
 */

/**
 * Windows.h should always be included first because other Windows header
 * files, such as KnownFolders.h, don't always define things properly if
 * you don't include Windows.h first.
 */
#include <Windows.h>
#undef CreateDirectory

#include "../FileImpl.hpp"

#include <SystemAbstractions/File.hpp>
#include <SystemAbstractions/StringExtensions.hpp>

#include <io.h>
#include <KnownFolders.h>
#include <memory>
#include <regex>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <stddef.h>
#include <stdio.h>

// Ensure we link with Windows shell utility libraries.
#pragma comment(lib, "Shlwapi")
#pragma comment(lib, "Shell32")

namespace {

    /**
     * This function replaces all backslashes with forward slashes
     * in the given string.
     *
     * @param[in] in
     *     This is the string to fix.
     *
     * @return
     *     A copy of the given string, with all backslashes replaced
     *     with forward slashes, is returned.
     */
    std::string FixPathDelimiters(const std::string& in) {
        std::string out;
        for (auto c: in) {
            if (c == '\\') {
                out.push_back('/');
            } else {
                out.push_back(c);
            }
        }
        return out;
    }

}

namespace SystemAbstractions {

    /**
     * This is the Win32-specific state for the File class.
     */
    struct File::Platform {
        /**
         * This is the operating-system handle to the file.
         */
        HANDLE handle;

        /**
         * This flag indicates whether or not the file was
         * opened with write access.
         */
        bool writeAccess = false;
    };

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
        std::string oneLevelUp(path.substr(0, delimiter));
        if (CreateDirectoryA(oneLevelUp.c_str(), NULL) != 0) {
            return true;
        }
        const DWORD error = GetLastError();
        if (error == ERROR_ALREADY_EXISTS) {
            return true;
        }
        if (error != ERROR_PATH_NOT_FOUND) {
            return false;
        }
        if (!CreatePath(oneLevelUp)) {
            return false;
        }
        if (CreateDirectoryA(oneLevelUp.c_str(), NULL) == 0) {
            return false;
        }
        return true;
    }

    File::~File() {
        if (impl_ == nullptr) {
            return;
        }
        Close();
    }

    File::File(File&& other) = default;
    File& File::operator=(File&& other) = default;

    File::File(std::string path)
        : impl_(new Impl())
    {
        impl_->path = path;
        impl_->platform->handle = INVALID_HANDLE_VALUE;
    }

    bool File::IsExisting() {
        const DWORD attr = GetFileAttributesA(impl_->path.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) {
            return false;
        }
        return true;
    }

    bool File::IsDirectory() {
        const DWORD attr = GetFileAttributesA(impl_->path.c_str());
        if (
            (attr == INVALID_FILE_ATTRIBUTES)
            || ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0)
        ) {
            return false;
        }
        return true;
    }

    bool File::Open() {
        Close();
        impl_->platform->handle = CreateFileA(
            impl_->path.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
        impl_->platform->writeAccess = false;
        return (impl_->platform->handle != INVALID_HANDLE_VALUE);
    }

    void File::Close() {
        (void)CloseHandle(impl_->platform->handle);
        impl_->platform->handle = INVALID_HANDLE_VALUE;
    }

    bool File::Create() {
        Close();
        bool createPathTried = false;
        while (impl_->platform->handle == INVALID_HANDLE_VALUE) {
            impl_->platform->handle = CreateFileA(
                impl_->path.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );
            if (impl_->platform->handle == INVALID_HANDLE_VALUE) {
                if (
                    createPathTried
                    || !impl_->CreatePath(impl_->path)
                ) {
                    return false;
                }
                createPathTried = true;
            }
            impl_->platform->writeAccess = true;
        }
        return true;
    }

    void File::Destroy() {
        Close();
        (void)DeleteFileA(impl_->path.c_str());
    }

    bool File::Move(const std::string& newPath) {
        if (MoveFileA(impl_->path.c_str(), newPath.c_str()) == 0) {
            return false;
        }
        impl_->path = newPath;
        return true;
    }

    bool File::Copy(const std::string& destination) {
        return (CopyFileA(impl_->path.c_str(), destination.c_str(), FALSE) != FALSE);
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
        static std::regex AbsolutePathRegex("[a-zA-Z]:[/\\\\].*");
        return std::regex_match(path, AbsolutePathRegex);
    }

    std::string File::GetExeImagePath() {
        std::vector< char > exeImagePath(MAX_PATH + 1);
        (void)GetModuleFileNameA(NULL, &exeImagePath[0], static_cast< DWORD >(exeImagePath.size()));
        return FixPathDelimiters(&exeImagePath[0]);
    }

    std::string File::GetExeParentDirectory() {
        std::vector< char > exeDirectory(MAX_PATH + 1);
        (void)GetModuleFileNameA(NULL, &exeDirectory[0], static_cast< DWORD >(exeDirectory.size()));
        (void)PathRemoveFileSpecA(&exeDirectory[0]);
        return FixPathDelimiters(&exeDirectory[0]);
    }

    std::string File::GetResourceFilePath(const std::string& name) {
        return SystemAbstractions::sprintf("%s/%s", GetExeParentDirectory().c_str(), name.c_str());
    }

    std::string File::GetLocalPerUserConfigDirectory(const std::string& nameKey) {
        PWSTR pathWide;
        if (SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &pathWide) != S_OK) {
            return "";
        }
        std::string pathNarrow(SystemAbstractions::wcstombs(pathWide));
        CoTaskMemFree(pathWide);
        return SystemAbstractions::sprintf("%s/%s", pathNarrow.c_str(), nameKey.c_str());
    }

    std::string File::GetUserSavedGamesDirectory(const std::string& nameKey) {
        PWSTR pathWide;
        if (SHGetKnownFolderPath(FOLDERID_SavedGames, 0, NULL, &pathWide) != S_OK) {
            return "";
        }
        std::string pathNarrow(SystemAbstractions::wcstombs(pathWide));
        CoTaskMemFree(pathWide);
        return SystemAbstractions::sprintf("%s/%s", pathNarrow.c_str(), nameKey.c_str());
    }

    void File::ListDirectory(const std::string& directory, std::vector< std::string >& list) {
        std::string directoryWithSeparator(directory);
        if (
            (directoryWithSeparator.length() > 0)
            && (directoryWithSeparator[directoryWithSeparator.length() - 1] != '\\')
            && (directoryWithSeparator[directoryWithSeparator.length() - 1] != '/')
        ) {
            directoryWithSeparator += '\\';
        }
        std::string listGlob(directoryWithSeparator);
        listGlob += "*.*";
        WIN32_FIND_DATAA findFileData;
        const HANDLE searchHandle = FindFirstFileA(listGlob.c_str(), &findFileData);
        list.clear();
        if (searchHandle != INVALID_HANDLE_VALUE) {
            do {
                std::string name(findFileData.cFileName);
                if (
                    (name == ".")
                    || (name == "..")
                ) {
                    continue;
                }
                std::string filePath(directoryWithSeparator);
                filePath += name;
                list.push_back(FixPathDelimiters(filePath));
            } while (FindNextFileA(searchHandle, &findFileData) == TRUE);
            FindClose(searchHandle);
        }
    }

    bool File::DeleteDirectory(const std::string& directory) {
        std::string directoryWithSeparator(directory);
        if (
            (directoryWithSeparator.length() > 0)
            && (directoryWithSeparator[directoryWithSeparator.length() - 1] != '\\')
            && (directoryWithSeparator[directoryWithSeparator.length() - 1] != '/')
        ) {
            directoryWithSeparator += '\\';
        }
        std::string listGlob(directoryWithSeparator);
        listGlob += "*.*";
        WIN32_FIND_DATAA findFileData;
        const HANDLE searchHandle = FindFirstFileA(listGlob.c_str(), &findFileData);
        if (searchHandle != INVALID_HANDLE_VALUE) {
            do {
                std::string name(findFileData.cFileName);
                if (
                    (name == ".")
                    || (name == "..")
                ) {
                    continue;
                }
                std::string filePath(directoryWithSeparator);
                filePath += name;
                if (PathIsDirectoryA(filePath.c_str())) {
                    if (!DeleteDirectory(filePath.c_str())) {
                        return false;
                    }
                } else {
                    if (DeleteFileA(filePath.c_str()) == 0) {
                        return false;
                    }
                }
            } while (FindNextFileA(searchHandle, &findFileData) == TRUE);
            FindClose(searchHandle);
        }
        return (RemoveDirectoryA(directory.c_str()) != 0);
    }

    bool File::CopyDirectory(
        const std::string& existingDirectory,
        const std::string& newDirectory
    ) {
        std::string existingDirectoryWithSeparator(existingDirectory);
        if (
            (existingDirectoryWithSeparator.length() > 0)
            && (existingDirectoryWithSeparator[existingDirectoryWithSeparator.length() - 1] != '\\')
            && (existingDirectoryWithSeparator[existingDirectoryWithSeparator.length() - 1] != '/')
        ) {
            existingDirectoryWithSeparator += '\\';
        }
        std::string newDirectoryWithSeparator(newDirectory);
        if (
            (newDirectoryWithSeparator.length() > 0)
            && (newDirectoryWithSeparator[newDirectoryWithSeparator.length() - 1] != '\\')
            && (newDirectoryWithSeparator[newDirectoryWithSeparator.length() - 1] != '/')
        ) {
            newDirectoryWithSeparator += '\\';
        }
        if (!File::Impl::CreatePath(newDirectoryWithSeparator)) {
            return false;
        }
        std::string listGlob(existingDirectoryWithSeparator);
        listGlob += "*.*";
        WIN32_FIND_DATAA findFileData;
        const HANDLE searchHandle = FindFirstFileA(listGlob.c_str(), &findFileData);
        if (searchHandle != INVALID_HANDLE_VALUE) {
            do {
                std::string name(findFileData.cFileName);
                if (
                    (name == ".")
                    || (name == "..")
                ) {
                    continue;
                }
                std::string filePath(existingDirectoryWithSeparator);
                filePath += name;
                std::string newFilePath(newDirectoryWithSeparator);
                newFilePath += name;
                if (PathIsDirectoryA(filePath.c_str())) {
                    if (!CopyDirectory(filePath, newFilePath)) {
                        return false;
                    }
                } else {
                    if (!CopyFileA(filePath.c_str(), newFilePath.c_str(), FALSE)) {
                        return false;
                    }
                }
            } while (FindNextFileA(searchHandle, &findFileData) == TRUE);
            FindClose(searchHandle);
        }
        return true;
    }

    std::vector< std::string > File::GetDirectoryRoots() {
        const auto driveStringsBufferSize = (size_t)GetLogicalDriveStringsA(0, nullptr);
        std::shared_ptr< char > driveStringsBuffer(
            new char[driveStringsBufferSize],
            [](char* p){ delete[] p; }
        );
        if (GetLogicalDriveStringsA((DWORD)driveStringsBufferSize, driveStringsBuffer.get()) > 0) {
            std::vector< std::string > roots;
            size_t i = 0;
            for (;;) {
                size_t j = i;
                while (driveStringsBuffer.get()[j] != 0) {
                    ++j;
                }
                if (i == j) {
                    break;
                }
                size_t k = j;
                while (
                    (k > i)
                    && (
                        (driveStringsBuffer.get()[k - 1] == '/')
                        || (driveStringsBuffer.get()[k - 1] == '\\')
                    )
                ) {
                    --k;
                }
                roots.emplace_back(
                    driveStringsBuffer.get() + i,
                    driveStringsBuffer.get() + k
                );
                i = j + 1;
            }
            return roots;
        } else {
            return {};
        }
    }

    uint64_t File::GetSize() const {
        LARGE_INTEGER size;
        if (GetFileSizeEx(impl_->platform->handle, &size) == 0) {
            return 0;
        }
        return (uint64_t)size.QuadPart;
    }

    bool File::SetSize(uint64_t size) {
        const uint64_t position = GetPosition();
        SetPosition(size);
        const bool success = (SetEndOfFile(impl_->platform->handle) != 0);
        SetPosition(position);
        return success;
    }

    uint64_t File::GetPosition() const {
        LARGE_INTEGER distanceToMove;
        distanceToMove.QuadPart = 0;
        LARGE_INTEGER newPosition;
        if (SetFilePointerEx(impl_->platform->handle, distanceToMove, &newPosition, FILE_CURRENT) == 0) {
            return 0;
        }
        return (uint64_t)newPosition.QuadPart;
    }

    void File::SetPosition(uint64_t position) {
        LARGE_INTEGER distanceToMove;
        distanceToMove.QuadPart = position;
        LARGE_INTEGER newPosition;
        (void)SetFilePointerEx(impl_->platform->handle, distanceToMove, &newPosition, FILE_BEGIN);
    }

    size_t File::Peek(void* buffer, size_t numBytes) const {
        const uint64_t position = GetPosition();
        DWORD amountRead;
        if (ReadFile(impl_->platform->handle, buffer, (DWORD)numBytes, &amountRead, NULL) == 0) {
            return 0;
        }
        LARGE_INTEGER distanceToMove;
        distanceToMove.QuadPart = position;
        LARGE_INTEGER newPosition;
        (void)SetFilePointerEx(impl_->platform->handle, distanceToMove, &newPosition, FILE_BEGIN);
        return (size_t)amountRead;
    }

    size_t File::Read(void* buffer, size_t numBytes) {
        if (numBytes == 0) {
            return 0;
        }
        DWORD amountRead;
        if (ReadFile(impl_->platform->handle, buffer, (DWORD)numBytes, &amountRead, NULL) == 0) {
            return 0;
        }
        return (size_t)amountRead;
    }

    size_t File::Write(const void* buffer, size_t numBytes) {
        DWORD amountWritten;
        if (WriteFile(impl_->platform->handle, buffer, (DWORD)numBytes, &amountWritten, NULL) == 0) {
            return 0;
        }
        return (size_t)amountWritten;
    }

    std::shared_ptr< IFile > File::Clone() {
        auto clone = std::make_shared< File >(impl_->path);
        clone->impl_->platform->writeAccess = impl_->platform->writeAccess;
        if (impl_->platform->handle != INVALID_HANDLE_VALUE) {
            if (clone->impl_->platform->writeAccess) {
                clone->impl_->platform->handle = CreateFileA(
                    clone->impl_->path.c_str(),
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                );
            } else {
                clone->impl_->platform->handle = CreateFileA(
                    clone->impl_->path.c_str(),
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                );
            }
            if (clone->impl_->platform->handle == INVALID_HANDLE_VALUE) {
                return nullptr;
            }
        }
        return clone;
    }

}
