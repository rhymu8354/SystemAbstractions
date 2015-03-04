/**
 * @file OSFileWin32.cpp
 *
 * This module contains the Win32 specific part of the 
 * implementation of the Files::OSFile class.
 *
 * Copyright (c) 2013-2014 by Richard Walters
 */

/**
 * Windows.h should always be included first because other Windows header
 * files, such as KnownFolders.h, don't always define things properly if
 * you don't include Windows.h first.
 */
#include <Windows.h>

#include "../OSFile.h"

#include <io.h>
#include <KnownFolders.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <stddef.h>
#include <stdio.h>
#include <StringExtensions/StringExtensions.h>

// Ensure we link with Windows shell utility libraries.
#pragma comment(lib, "Shlwapi")
#pragma comment(lib, "Shell32")

namespace Files {

    /**
     * This is the Win32-specific state for the OSFile class.
     */
    struct OSFileImpl {
        HANDLE handle;
    };

    OSFile::OSFile(std::string path)
        : _path(path)
        , _impl(new OSFileImpl())
    {
        _impl->handle = INVALID_HANDLE_VALUE;
    }

    OSFile::~OSFile() {
        Close();
    }

    bool OSFile::IsExisting() {
        const DWORD attr = GetFileAttributesA(_path.c_str());
        if (attr == INVALID_FILE_ATTRIBUTES) {
            return false;
        }
        if (attr & FILE_ATTRIBUTE_DIRECTORY) {
            return false;
        }
        return true;
    }

    bool OSFile::Open() {
        Close();
        _impl->handle = CreateFileA(
            _path.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
        return (_impl->handle != INVALID_HANDLE_VALUE);
    }

    void OSFile::Close() {
        (void)CloseHandle(_impl->handle);
        _impl->handle = INVALID_HANDLE_VALUE;
    }

    bool OSFile::Create() {
        Close();
        bool createPathTried = false;
        while (_impl->handle == INVALID_HANDLE_VALUE) {
            _impl->handle = CreateFileA(
                _path.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );
            if (_impl->handle == INVALID_HANDLE_VALUE) {
                if (
                    createPathTried
                    || !CreatePath(_path)
                ) {
                    return false;
                }
                createPathTried = true;
            }
        }
        return true;
    }

    void OSFile::Destroy() {
        Close();
        (void)DeleteFileA(_path.c_str());
    }

    std::string OSFile::GetExeDirectory() {
        std::vector< char > exeDirectory(MAX_PATH + 1);
        GetModuleFileNameA(NULL, &exeDirectory[0], static_cast< DWORD >(exeDirectory.size()));
        PathRemoveFileSpecA(&exeDirectory[0]);
        return std::string(&exeDirectory[0]);
    }

    std::string OSFile::GetResourceFilePath(const std::string& name) {
        return StringExtensions::sprintf("%s/%s", GetExeDirectory().c_str(), name.c_str());
    }

    std::string OSFile::GetUserSavedGamesDirectory(const std::string& nameKey) {
        PWSTR pathWide;
        if (SHGetKnownFolderPath(FOLDERID_SavedGames, 0, NULL, &pathWide) != S_OK) {
            return "";
        }
        std::string pathNarrow(StringExtensions::wcstombs(pathWide));
        CoTaskMemFree(pathWide);
        return StringExtensions::sprintf("%s/%s", pathNarrow.c_str(), nameKey.c_str());
    }

    void OSFile::ListDirectory(const std::string& directory, std::vector< std::string >& list) {
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
                std::string filePath(directoryWithSeparator);
                filePath += findFileData.cFileName;
                list.push_back(filePath);
            } while (FindNextFileA(searchHandle, &findFileData) == TRUE);
            FindClose(searchHandle);
        }
    }

    void OSFile::DeleteDirectory(const std::string& directory) {
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
                    DeleteDirectory(filePath.c_str());
                } else {
                    (void)DeleteFileA(filePath.c_str());
                }
            } while (FindNextFileA(searchHandle, &findFileData) == TRUE);
            FindClose(searchHandle);
        }
        (void)RemoveDirectoryA(directory.c_str());
    }

    uint64_t OSFile::GetSize() const {
        LARGE_INTEGER size;
        if (GetFileSizeEx(_impl->handle, &size) == 0) {
            return 0;
        }
        return (uint64_t)size.QuadPart;
    }

    bool OSFile::SetSize(uint64_t size) {
        const uint64_t position = GetPosition();
        SetPosition(size);
        const bool success = (SetEndOfFile(_impl->handle) != 0);
        SetPosition(position);
        return success;
    }

    uint64_t OSFile::GetPosition() const {
        LARGE_INTEGER distanceToMove;
        distanceToMove.QuadPart = 0;
        LARGE_INTEGER newPosition;
        if (SetFilePointerEx(_impl->handle, distanceToMove, &newPosition, FILE_CURRENT) == 0) {
            return 0;
        }
        return (uint64_t)newPosition.QuadPart;
    }

    void OSFile::SetPosition(uint64_t position) {
        LARGE_INTEGER distanceToMove;
        distanceToMove.QuadPart = position;
        LARGE_INTEGER newPosition;
        (void)SetFilePointerEx(_impl->handle, distanceToMove, &newPosition, FILE_BEGIN);
    }

    size_t OSFile::Peek(void* buffer, size_t numBytes) const {
        const uint64_t position = GetPosition();
        DWORD amountRead;
        if (ReadFile(_impl->handle, buffer, (DWORD)numBytes, &amountRead, NULL) == 0) {
            return 0;
        }
        LARGE_INTEGER distanceToMove;
        distanceToMove.QuadPart = position;
        LARGE_INTEGER newPosition;
        (void)SetFilePointerEx(_impl->handle, distanceToMove, &newPosition, FILE_BEGIN);
        return (size_t)amountRead;
    }

    size_t OSFile::Read(void* buffer, size_t numBytes) {
        if (numBytes == 0) {
            return 0;
        }
        DWORD amountRead;
        if (ReadFile(_impl->handle, buffer, (DWORD)numBytes, &amountRead, NULL) == 0) {
            return 0;
        }
        return (size_t)amountRead;
    }

    size_t OSFile::Write(const void* buffer, size_t numBytes) {
        DWORD amountWritten;
        if (WriteFile(_impl->handle, buffer, (DWORD)numBytes, &amountWritten, NULL) == 0) {
            return 0;
        }
        return (size_t)amountWritten;
    }

    bool OSFile::CreatePath(std::string path) {
        const size_t delimiter = path.find_last_of("/\\");
        if (delimiter == std::string::npos) {
            return false;
        }
        std::string oneLevelUp(path.substr(0, delimiter));
        if (CreateDirectoryA(oneLevelUp.c_str(), NULL) != 0) {
            return true;
        }
        const DWORD error = GetLastError();
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

}
