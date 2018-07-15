/**
 * @file NetworkConnectionWin32.cpp
 *
 * This module contains the Windows implementation of the
 * SystemAbstractions::NetworkConnection::Impl class.
 *
 * © 2016-2018 by Richard Walters
 */

/**
 * WinSock2.h should always be included first because if Windows.h is
 * included before it, WinSock.h gets included which conflicts
 * with WinSock2.h.
 *
 * Windows.h should always be included next because other Windows header
 * files, such as KnownFolders.h, don't always define things properly if
 * you don't include Windows.h beforehand.
 */
#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32")
#undef ERROR
#undef SendMessage
#undef min
#undef max

#include "../NetworkConnectionImpl.hpp"
#include "NetworkConnectionWin32.hpp"

#include <algorithm>
#include <deque>
#include <inttypes.h>
#include <mutex>
#include <stdint.h>
#include <thread>

namespace {

    /**
     * This is the maximum number of bytes to try to read
     * from a network socket at once.
     */
    static const size_t MAXIMUM_READ_SIZE = 65536;

    /**
     * This is the maximum number of bytes to try to write
     * to a network socket at once.
     */
    static const size_t MAXIMUM_WRITE_SIZE = 65536;

}

namespace SystemAbstractions {

    NetworkConnection::Impl::~Impl() {
        Close(true);
        if (platform->wsaStarted) {
            (void)WSACleanup();
        }
        if (platform->socketEvent != NULL) {
            (void)CloseHandle(platform->socketEvent);
        }
        if (platform->processorStateChangeEvent != NULL) {
            (void)CloseHandle(platform->processorStateChangeEvent);
        }
    }

    NetworkConnection::Impl::Impl()
        : platform(new NetworkConnection::Platform())
        , diagnosticsSender("NetworkConnection")
    {
        WSADATA wsaData;
        if (!WSAStartup(MAKEWORD(2, 0), &wsaData)) {
            platform->wsaStarted = true;
        }
    }

    bool NetworkConnection::Impl::Connect() {
        Close(true);
        struct sockaddr_in socketAddress;
        (void)memset(&socketAddress, 0, sizeof(socketAddress));
        socketAddress.sin_family = AF_INET;
        platform->sock = socket(socketAddress.sin_family, SOCK_STREAM, 0);
        if (platform->sock == INVALID_SOCKET) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "error creating socket (%d)",
                WSAGetLastError()
            );
            return false;
        }
        if (bind(platform->sock, (struct sockaddr*)&socketAddress, sizeof(socketAddress)) != 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "error in bind (%d)",
                WSAGetLastError()
            );
            Close(false);
            return false;
        }
        (void)memset(&socketAddress, 0, sizeof(socketAddress));
        socketAddress.sin_family = AF_INET;
        socketAddress.sin_addr.S_un.S_addr = htonl(peerAddress);
        socketAddress.sin_port = htons(peerPort);
        if (connect(platform->sock, (const sockaddr*)&socketAddress, sizeof(socketAddress)) != 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "error in connect (%d)",
                WSAGetLastError()
            );
            Close(false);
            return false;
        }
        return true;
    }

    bool NetworkConnection::Impl::Process() {
        if (platform->sock == INVALID_SOCKET) {
            diagnosticsSender.SendDiagnosticInformationString(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "not connected"
            );
            return false;
        }
        if (platform->processor.joinable()) {
            diagnosticsSender.SendDiagnosticInformationString(
                SystemAbstractions::DiagnosticsSender::Levels::WARNING,
                "already processing"
            );
            return true;
        }
        platform->processorStop = false;
        if (platform->processorStateChangeEvent == NULL) {
            platform->processorStateChangeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (platform->processorStateChangeEvent == NULL) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                    "error creating processor state change event (%d)",
                    (int)GetLastError()
                );
                return false;
            }
        }
        if (platform->socketEvent == NULL) {
            platform->socketEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (platform->socketEvent == NULL) {
                diagnosticsSender.SendDiagnosticInformationFormatted(
                    SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                    "error creating socket event (%d)",
                    (int)GetLastError()
                );
                return false;
            }
        }
        if (WSAEventSelect(platform->sock, platform->socketEvent, FD_READ | FD_WRITE | FD_CLOSE) != 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "error in WSAEventSelect for FD_READ (%d)",
                WSAGetLastError()
            );
            return false;
        }
        platform->processor = std::move(std::thread(&NetworkConnection::Impl::Processor, this));
        return true;
    }

    void NetworkConnection::Impl::Processor() {
        const HANDLE handles[2] = { platform->processorStateChangeEvent, platform->socketEvent };
        std::vector< uint8_t > buffer;
        std::unique_lock< std::recursive_mutex > processingLock(platform->processingMutex);
        bool wait = true;
        while (!platform->processorStop) {
            if (wait) {
                processingLock.unlock();
                (void)WaitForMultipleObjects(2, handles, FALSE, INFINITE);
                processingLock.lock();
            }
            buffer.resize(MAXIMUM_READ_SIZE);
            const int amountReceived = recv(platform->sock, (char*)&buffer[0], (int)buffer.size(), 0);
            if (amountReceived == SOCKET_ERROR) {
                const auto wsaLastError = WSAGetLastError();
                if (wsaLastError == WSAEWOULDBLOCK) {
                    wait = true;
                } else {
                    Close(false);
                    brokenDelegate();
                    break;
                }
            } else if (amountReceived > 0) {
                wait = false;
                buffer.resize((size_t)amountReceived);
                messageReceivedDelegate(buffer);
            } else {
                Close(false);
                brokenDelegate();
                break;
            }
            const auto outputQueueLength = platform->outputQueue.size();
            if (outputQueueLength > 0) {
                const auto writeSize = (int)std::min(outputQueueLength, MAXIMUM_WRITE_SIZE);
                buffer.assign(
                    platform->outputQueue.begin(),
                    platform->outputQueue.begin() + writeSize
                );
                const int amountSent = send(platform->sock, (const char*)&buffer[0], writeSize, 0);
                if (amountSent == SOCKET_ERROR) {
                    const auto wsaLastError = WSAGetLastError();
                    if (wsaLastError != WSAEWOULDBLOCK) {
                        Close(false);
                        brokenDelegate();
                        break;
                    }
                } else if (amountSent > 0) {
                    (void)platform->outputQueue.erase(platform->outputQueue.begin(), platform->outputQueue.begin() + amountSent);
                    if (
                        (amountSent == writeSize)
                        && !platform->outputQueue.empty()
                    ) {
                        wait = false;
                    }
                } else {
                    Close(false);
                    brokenDelegate();
                    break;
                }
            }
        }
    }

    bool NetworkConnection::Impl::IsConnected() const {
        return (platform->sock != INVALID_SOCKET);
    }

    void NetworkConnection::Impl::SendMessage(const std::vector< uint8_t >& message) {
        std::unique_lock< std::recursive_mutex > processingLock(platform->processingMutex);
        platform->outputQueue.insert(platform->outputQueue.end(), message.begin(), message.end());
        (void)SetEvent(platform->processorStateChangeEvent);
    }

    void NetworkConnection::Impl::Close(bool stopProcessing) {
        if (
            stopProcessing
            && platform->processor.joinable()
        ) {
            platform->processorStop = true;
            (void)SetEvent(platform->processorStateChangeEvent);
            platform->processor.join();
        }
        std::unique_lock< std::recursive_mutex > processingLock(platform->processingMutex);
        if (platform->sock != INVALID_SOCKET) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                0,
                "closing connection with %" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8 ":%" PRIu16,
                (uint8_t)((peerAddress >> 24) & 0xFF),
                (uint8_t)((peerAddress >> 16) & 0xFF),
                (uint8_t)((peerAddress >> 8) & 0xFF),
                (uint8_t)(peerAddress & 0xFF),
                peerPort
            );
            (void)closesocket(platform->sock);
            platform->sock = INVALID_SOCKET;
        }
    }

    std::shared_ptr< NetworkConnection > NetworkConnection::Platform::MakeConnectionFromExistingSocket(
        SOCKET sock,
        uint32_t address,
        uint16_t port
    ) {
        const auto connection = std::make_shared< NetworkConnection >();
        connection->impl_->platform->sock = sock;
        connection->impl_->peerAddress = address;
        connection->impl_->peerPort = port;
        return connection;
    }

}
