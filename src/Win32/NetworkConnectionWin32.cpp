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
#include <functional>
#include <inttypes.h>
#include <mutex>
#include <stdint.h>
#include <string.h>
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
        Close(CloseProcedure::ImmediateAndStopProcessor);
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
        Close(CloseProcedure::ImmediateAndStopProcessor);
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
        LINGER linger;
        linger.l_onoff = 1;
        linger.l_linger = 0;
        (void)setsockopt(platform->sock, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(linger));
        if (bind(platform->sock, (struct sockaddr*)&socketAddress, sizeof(socketAddress)) != 0) {
            diagnosticsSender.SendDiagnosticInformationFormatted(
                SystemAbstractions::DiagnosticsSender::Levels::ERROR,
                "error in bind (%d)",
                WSAGetLastError()
            );
            Close(CloseProcedure::ImmediateDoNotStopProcessor);
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
            Close(CloseProcedure::ImmediateDoNotStopProcessor);
            return false;
        }
        int socketAddressLength = sizeof(socketAddress);
        if (getsockname(platform->sock, (struct sockaddr*)&socketAddress, &socketAddressLength) == 0) {
            boundAddress = ntohl(socketAddress.sin_addr.S_un.S_addr);
            boundPort = ntohs(socketAddress.sin_port);
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
                "error in WSAEventSelect (%d)",
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
        while (
            !platform->processorStop
            && (platform->sock != INVALID_SOCKET)
        ) {
            if (wait) {
                diagnosticsSender.SendDiagnosticInformationString(0, "processor going to sleep");
                processingLock.unlock();
                (void)WaitForMultipleObjects(2, handles, FALSE, INFINITE);
                processingLock.lock();
            }
            diagnosticsSender.SendDiagnosticInformationString(0, "processor woke up");
            if (platform->peerClosed) {
                wait = true;
            } else {
                buffer.resize(MAXIMUM_READ_SIZE);
                diagnosticsSender.SendDiagnosticInformationString(0, "processor trying to read");
                const int amountReceived = recv(platform->sock, (char*)&buffer[0], (int)buffer.size(), 0);
                if (amountReceived == SOCKET_ERROR) {
                    const auto wsaLastError = WSAGetLastError();
                    if (wsaLastError == WSAEWOULDBLOCK) {
                        wait = true;
                    } else {
                        diagnosticsSender.SendDiagnosticInformationString(
                            1,
                            "connection with " + GetPeerName() + " closed abruptly by peer"
                        );
                        Close(CloseProcedure::ImmediateDoNotStopProcessor);
                        break;
                    }
                } else if (amountReceived > 0) {
                    diagnosticsSender.SendDiagnosticInformationString(0, "processor read something");
                    wait = false;
                    buffer.resize((size_t)amountReceived);
                    messageReceivedDelegate(buffer);
                } else {
                    diagnosticsSender.SendDiagnosticInformationString(
                        1,
                        "connection with " + GetPeerName() + " closed gracefully by peer"
                    );
                    platform->peerClosed = true;
                    brokenDelegate(true);
                }
            }
            if (platform->sock == INVALID_SOCKET) {
                break;
            }
            const auto outputQueueLength = platform->outputQueue.GetBytesQueued();
            if (outputQueueLength > 0) {
                diagnosticsSender.SendDiagnosticInformationString(0, "processor trying to write");
                const auto writeSize = (int)std::min(outputQueueLength, MAXIMUM_WRITE_SIZE);
                buffer = platform->outputQueue.Peek(writeSize);
                const int amountSent = send(platform->sock, (const char*)&buffer[0], writeSize, 0);
                if (amountSent == SOCKET_ERROR) {
                    const auto wsaLastError = WSAGetLastError();
                    if (wsaLastError != WSAEWOULDBLOCK) {
                        diagnosticsSender.SendDiagnosticInformationString(
                            1,
                            "connection with " + GetPeerName() + " closed abruptly by peer"
                        );
                        Close(CloseProcedure::ImmediateDoNotStopProcessor);
                        diagnosticsSender.SendDiagnosticInformationString(0, "processor breaking due to send error");
                        break;
                    }
                } else if (amountSent > 0) {
                    diagnosticsSender.SendDiagnosticInformationString(0, "processor wrote something");
                    (void)platform->outputQueue.Drop(amountSent);
                    if (
                        (amountSent == writeSize)
                        && (platform->outputQueue.GetBytesQueued() > 0)
                    ) {
                        diagnosticsSender.SendDiagnosticInformationString(0, "processor has more to write");
                        wait = false;
                    }
                } else {
                    Close(CloseProcedure::ImmediateDoNotStopProcessor);
                    diagnosticsSender.SendDiagnosticInformationString(0, "processor breaking due to send returning 0");
                    break;
                }
            }
            if (
                (platform->outputQueue.GetBytesQueued() == 0)
                && platform->closing
            ) {
                if (!platform->shutdownSent) {
                    diagnosticsSender.SendDiagnosticInformationString(0, "processor closing and done sending");
                    shutdown(platform->sock, SD_SEND);
                    platform->shutdownSent = true;
                }
                if (platform->peerClosed) {
                    diagnosticsSender.SendDiagnosticInformationString(0, "processor closing connection immediately");
                    CloseImmediately();
                }
            }
        }
        diagnosticsSender.SendDiagnosticInformationString(0, "processor returning due to being told to stop");
    }

    bool NetworkConnection::Impl::IsConnected() const {
        return (platform->sock != INVALID_SOCKET);
    }

    void NetworkConnection::Impl::SendMessage(const std::vector< uint8_t >& message) {
        std::unique_lock< std::recursive_mutex > processingLock(platform->processingMutex);
        platform->outputQueue.Enqueue(message);
        (void)SetEvent(platform->processorStateChangeEvent);
    }

    void NetworkConnection::Impl::Close(CloseProcedure procedure) {
        if (
            (procedure == CloseProcedure::ImmediateAndStopProcessor)
            && platform->processor.joinable()
            && (std::this_thread::get_id() != platform->processor.get_id())
        ) {
            platform->processorStop = true;
            (void)SetEvent(platform->processorStateChangeEvent);
            platform->processor.join();
        }
        std::unique_lock< std::recursive_mutex > processingLock(platform->processingMutex);
        if (platform->sock != INVALID_SOCKET) {
            if (procedure == CloseProcedure::Graceful) {
                platform->closing = true;
                diagnosticsSender.SendDiagnosticInformationString(
                    1,
                    "closing connection with " + GetPeerName()
                );
                (void)SetEvent(platform->processorStateChangeEvent);
            } else {
                CloseImmediately();
            }
        }
    }

    void NetworkConnection::Impl::CloseImmediately() {
        platform->CloseImmediately();
        diagnosticsSender.SendDiagnosticInformationString(
            1,
            "closed connection with " + GetPeerName()
        );
        if (brokenDelegate != nullptr) {
            brokenDelegate(false);
        }
    }

    uint32_t NetworkConnection::Impl::GetAddressOfHost(const std::string& host) {
        bool wsaStarted = false;
        const std::unique_ptr< WSADATA, std::function< void(WSADATA*) > > wsaData(
            new WSADATA,
            [&wsaStarted](WSADATA* p){
                if (wsaStarted) {
                    (void)WSACleanup();
                }
                delete p;
            }
        );
        wsaStarted = !WSAStartup(MAKEWORD(2, 0), wsaData.get());
        struct addrinfo hints;
        (void)memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        struct addrinfo* rawResults;
        if (getaddrinfo(host.c_str(), NULL, &hints, &rawResults) != 0) {
            return 0;
        }
        std::unique_ptr< struct addrinfo, std::function< void(struct addrinfo*) > > results(
            rawResults,
            [](struct addrinfo* p){
                freeaddrinfo(p);
            }
        );
        if (results == NULL) {
            return 0;
        } else {
            struct sockaddr_in* ipAddress = (struct sockaddr_in*)results->ai_addr;
            return ntohl(ipAddress->sin_addr.S_un.S_addr);
        }
    }

    std::shared_ptr< NetworkConnection > NetworkConnection::Platform::MakeConnectionFromExistingSocket(
        SOCKET sock,
        uint32_t boundAddress,
        uint16_t boundPort,
        uint32_t peerAddress,
        uint16_t peerPort
    ) {
        const auto connection = std::make_shared< NetworkConnection >();
        connection->impl_->platform->sock = sock;
        connection->impl_->boundAddress = boundAddress;
        connection->impl_->boundPort = boundPort;
        connection->impl_->peerAddress = peerAddress;
        connection->impl_->peerPort = peerPort;
        return connection;
    }

    void NetworkConnection::Platform::CloseImmediately() {
        (void)closesocket(sock);
        sock = INVALID_SOCKET;
    }

}
