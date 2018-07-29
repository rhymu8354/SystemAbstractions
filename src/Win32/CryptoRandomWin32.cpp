/**
 * @file CryptoRandomWin32.cpp
 *
 * This module contains the Win32 specific part of the
 * implementation of the SystemAbstractions::CryptoRandom class.
 *
 * © 2018 by Richard Walters
 */

/**
 * Windows.h should always be included first because other Windows header
 * files, such as KnownFolders.h, don't always define things properly if
 * you don't include Windows.h first.
 */
#include <Windows.h>

#include <bcrypt.h>
#include <SystemAbstractions/CryptoRandom.hpp>

#pragma comment(lib, "Bcrypt")

namespace SystemAbstractions {

    /**
     * This is the Win32-specific state for the CryptoRandom class.
     */
    struct CryptoRandom::Impl {
    };

    CryptoRandom::~CryptoRandom() = default;

    CryptoRandom::CryptoRandom()
        : impl_(new Impl())
    {
    }

    void CryptoRandom::Generate(void* buffer, size_t length) {
        (void)BCryptGenRandom(
            NULL, // required when dwFlags includes BCRYPT_USE_SYSTEM_PREFERRED_RNG
            (PUCHAR)buffer,
            (ULONG)length,
            BCRYPT_USE_SYSTEM_PREFERRED_RNG
        );
    }

}
