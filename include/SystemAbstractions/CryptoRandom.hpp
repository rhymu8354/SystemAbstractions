#ifndef SYSTEM_ABSTRACTIONS_CRYPTO_RANDOM_HPP
#define SYSTEM_ABSTRACTIONS_CRYPTO_RANDOM_HPP

/**
 * @file CryptoRandom.hpp
 *
 * This module declares the SystemAbstractions::CryptoRandom class.
 *
 * © 2018 by Richard Walters
 */

#include <memory>
#include <stddef.h>

namespace SystemAbstractions {

    /**
     * This class represents a generator of strong entropy random numbers.
     */
    class CryptoRandom {
        // Lifecycle methods
    public:
        ~CryptoRandom() noexcept;
        CryptoRandom(const CryptoRandom&) = delete;
        CryptoRandom(CryptoRandom&&) noexcept = delete;
        CryptoRandom& operator=(const CryptoRandom&) = delete;
        CryptoRandom& operator=(CryptoRandom&&) noexcept = delete;

        // Public methods
    public:
        /**
         * This is the default constructor.
         */
        CryptoRandom();

        /**
         * This method generates strong entropy random numbers and stores
         * them into the given buffer.
         *
         * @param[in] buffer
         *     This points to a buffer in which to store the numbers.
         *
         * @param[in] length
         *     This is the length of the buffer, in bytes.
         */
        void Generate(void* buffer, size_t length);

        // Private properties
    private:
        /**
         * This contains any platform-specific state for the object.
         */
        struct Impl;

        /**
         * This contains any platform-specific state for the object.
         */
        std::unique_ptr< Impl > impl_;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_CRYPTO_RANDOM_HPP */
