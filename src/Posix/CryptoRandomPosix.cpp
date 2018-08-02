/**
 * @file CryptoRandomPosix.cpp
 *
 * This module contains the POSIX specific part of the
 * implementation of the SystemAbstractions::CryptoRandom class.
 *
 * © 2018 by Richard Walters
 */

#include <fcntl.h>
#include <stdio.h>
#include <SystemAbstractions/CryptoRandom.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace SystemAbstractions {

    /**
     * This is the POSIX-specific state for the CryptoRandom class.
     */
    struct CryptoRandom::Impl {
        /**
         * This is used to read the random number generator device.
         */
        int rng;
    };

    CryptoRandom::~CryptoRandom() {
        (void)close(impl_->rng);
    }

    CryptoRandom::CryptoRandom()
        : impl_(new Impl())
    {
        impl_->rng = open("/dev/urandom", O_RDONLY);
    }

    void CryptoRandom::Generate(void* buffer, size_t length) {
        (void)read(impl_->rng, buffer, length);
    }

}
