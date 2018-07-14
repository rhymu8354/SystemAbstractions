#ifndef SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_WIN32_HPP
#define SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_WIN32_HPP

/**
 * @file NetworkEndpointWin32.hpp
 *
 * This module declares the Windows implementation of the
 * SystemAbstractions::NetworkEndpointPlatform structure.
 *
 * © 2016-2018 by Richard Walters
 */

#include <SystemAbstractions/NetworkEndpoint.hpp>

#include <list>
#include <mutex>
#include <stdint.h>
#include <thread>
#include <vector>

namespace SystemAbstractions {

    /**
     * This is the Win32-specific state for the NetworkEndpoint class.
     */
    struct NetworkEndpoint::Platform {
        /**
         * This is used to hold all the information about
         * a datagram to be sent.
         */
        struct Packet {
            /**
             * This is the IPv4 address of the datagram recipient.
             */
            uint32_t address;

            /**
             * This is the port number of the datagram recipient.
             */
            uint16_t port;

            /**
             * This is the message to send in the datagram.
             */
            std::vector< uint8_t > body;
        };

        /**
         * This keeps track of whether or not WSAStartup succeeded,
         * because if so we need to call WSACleanup upon teardown.
         */
        bool wsaStarted = false;

        /**
         * This is the operating system handle to the network
         * port bound by this object.
         */
        SOCKET sock = INVALID_SOCKET;

        /**
         * This is the thread which performs all the actual
         * sending and receiving of data over the network.
         */
        std::thread processor;

        /**
         * This is an event used with WSAEventSelect in order
         * for the worker thread to wait for interesting things
         * to happen with the bound network port.
         */
        HANDLE socketEvent = NULL;

        /**
         * This is an event used to wake up the worker thread
         * if a new message to send has been placed in the
         * output queue, or if we want the worker thread to stop.
         */
        HANDLE processorStateChangeEvent = NULL;

        /**
         * This flag indicates whether or not the worker thread
         * should stop.
         */
        bool processorStop = false;

        /**
         * This is used to synchronize access to the object.
         */
        std::recursive_mutex processingMutex;

        /**
         * This temporarily holds messages to be sent across the network
         * by the worker thread.  It is filled by the SendPacket method.
         */
        std::list< Packet > outputQueue;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_NETWORK_ENDPOINT_WIN32_HPP */
