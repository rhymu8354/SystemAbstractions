#ifndef SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_IMPL_HPP
#define SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_IMPL_HPP

/**
 * @file NetworkConnectionImpl.hpp
 *
 * This module declares the SystemAbstractions::NetworkConnectionImpl
 * structure.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "DiagnosticsSender.hpp"
#include "NetworkConnection.hpp"

#include <memory>
#include <stdint.h>
#include <vector>

namespace SystemAbstractions {

    /**
     * This structure contains the private properties of the
     * NetworkConnection class.
     */
    struct NetworkConnection::Impl {
        /**
         * @todo Needs documentation
         */
        std::unique_ptr< Platform > platform;

        /**
         * @todo Needs documentation
         */
        NetworkConnection::Owner* owner = nullptr;

        /**
         * @todo Needs documentation
         */
        uint32_t peerAddress = 0;

        /**
         * @todo Needs documentation
         */
        uint16_t peerPort = 0;

        /**
         * @todo Needs documentation
         */
        DiagnosticsSender diagnosticsSender;

        /**
         * This is the instance constructor.
         */
        Impl();

        /**
         * This is the instance destructor.
         */
        ~Impl();

        /**
         * @todo Needs documentation
         */
        bool Connect();

        /**
         * @todo Needs documentation
         */
        bool Process();

        /**
         * @todo Needs documentation
         */
        void Processor();

        /**
         * @todo Needs documentation
         */
        bool IsConnected() const;

        /**
         * @todo Needs documentation
         */
        void SendMessage(const std::vector< uint8_t >& message);

        /**
         * @todo Needs documentation
         */
        void Close(bool stopProcessing);
    };

}

#endif /* SYSTEM_ABSTRACTIONS_NETWORK_CONNECTION_IMPL_HPP */
