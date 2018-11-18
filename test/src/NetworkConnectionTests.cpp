/**
 * @file NetworkConnectionTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::NetworkConnection class.
 *
 * © 2018 by Richard Walters
 */

#include <algorithm>
#include <condition_variable>
#include <functional>
#include <gtest/gtest.h>
#include <mutex>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <SystemAbstractions/NetworkConnection.hpp>
#include <SystemAbstractions/NetworkEndpoint.hpp>
#include <SystemAbstractions/StringExtensions.hpp>
#include <thread>
#include <time.h>
#include <vector>

#ifdef _WIN32
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
#include <IPHlpApi.h>
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "IPHlpApi")
#undef ERROR
#undef SendMessage
#undef min
#undef max
#define IPV4_ADDRESS_IN_SOCKADDR sin_addr.S_un.S_addr
#define SOCKADDR_LENGTH_TYPE int
#else /* POSIX */
#include <fcntl.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#define IPV4_ADDRESS_IN_SOCKADDR sin_addr.s_addr
#define SOCKADDR_LENGTH_TYPE socklen_t
#define SOCKET int
#define closesocket close
#endif /* _WIN32 or POSIX */

namespace {

    /**
     * This is used to hold all the information about a received
     * datagram that we care about.
     */
    struct Packet {
        /**
         * This is a copy of the data from the received datagram.
         */
        std::vector< uint8_t > payload;

        /**
         * This is the IPv4 address of the datagram sender.
         */
        uint32_t address;

        /**
         * This is the port number of the datagram sender.
         */
        uint16_t port;

        /**
         * This constructor initializes all properies of the structure.
         *
         * @param[in] newPayload
         *     This is a copy of the data from the received datagram.
         *
         * @param[in] newAddress
         *     This is the IPv4 address of the datagram sender.
         *
         * @param[in] port
         *     This is the port number of the datagram sender.
         */
        Packet(
            const std::vector< uint8_t >& newPayload,
            uint32_t newAddress,
            uint16_t newPort
        )
            : payload(newPayload)
            , address(newAddress)
            , port(newPort)
        {
        }
    };

    /**
     * This is used to receive callbacks from the unit under test.
     */
    struct Owner {
        // Properties

        /**
         * This is used to wait for, or signal, a condition
         * upon which that the owner might be waiting.
         */
        std::condition_variable_any condition;

        /**
         * This is used to synchronize access to the class.
         */
        std::mutex mutex;

        /**
         * This holds a copy of each packet received.
         */
        std::vector< Packet > packetsReceived;

        /**
         * This holds the data received from a connection-oriented stream.
         */
        std::vector< uint8_t > streamReceived;

        /**
         * These are connections that have been established
         * between the unit under test and remote clients.
         */
        std::vector< std::shared_ptr< SystemAbstractions::NetworkConnection > > connections;

        /**
         * This flag indicates whether or not a connection
         * to the network endpoint has been broken.
         */
        bool connectionBroken = false;

        /**
         * This flag indicates whether or not a connection
         * to the network endpoint has been broken gracefully.
         */
        bool connectionBrokenGracefully = false;

        /**
         * This is a function to call when the connection is closed.
         *
         * @param[in] graceful
         *     This indicates whether or not the peer of connection
         *     has closed the connection gracefully (meaning we can
         *     continue to send our data back to the peer).
         */
        std::function< void(bool graceful) > connectionBrokenDelegate;

        // Methods

        /**
         * This method waits up to a second for a packet
         * to be received from the network endpoint.
         *
         * @return
         *     An indication of whether or not a packet
         *     is received from the network endpoint
         *     is returned.
         */
        bool AwaitPacket() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this]{
                    return !packetsReceived.empty();
                }
            );
        }

        /**
         * This method waits up to a second for the given number
         * of bytes to be received from a client connected
         * to the network endpoint.
         *
         * @param[in] numBytes
         *     This is the number of bytes we expect to receive.
         *
         * @return
         *     An indication of whether or not the given number
         *     of bytes were received from a client connected
         *     to the network endpoint is returned.
         */
        bool AwaitStream(size_t numBytes) {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this, numBytes]{
                    return (streamReceived.size() >= numBytes);
                }
            );
        }

        /**
         * This method waits up to a second for a connection
         * to be received at the network endpoint.
         *
         * @return
         *     An indication of whether or not a connection
         *     is received at the network endpoint
         *     is returned.
         */
        bool AwaitConnection() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this]{
                    return !connections.empty();
                }
            );
        }

        /**
         * This method waits up to a second for the given number
         * of connections to be received at the network endpoint.
         *
         * @param[in] numConnections
         *     This is the number of connections for which to wait.
         *
         * @return
         *     An indication of whether or not the given number
         *     of connections is received at the network endpoint
         *     is returned.
         */
        bool AwaitConnections(size_t numConnections) {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this, numConnections]{
                    return (connections.size() >= numConnections);
                }
            );
        }

        /**
         * This method waits up to a second for a connection
         * to the network endpoint to be broken.
         *
         * @return
         *     An indication of whether or not a connection
         *     to the network endpoint is broken
         *     is returned.
         */
        bool AwaitDisconnection() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            return condition.wait_for(
                lock,
                std::chrono::seconds(1),
                [this]{
                    return connectionBroken;
                }
            );
        }

        /**
         * This is the callback function to be called whenever
         * a new client connects to the network endpoint.
         *
         * @param[in] newConnection
         *     This represents the connection to the new client.
         */
        void NetworkConnectionNewConnection(std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection) {
            std::unique_lock< decltype(mutex) > lock(mutex);
            connections.push_back(newConnection);
            condition.notify_all();
            (void)newConnection->Process(
                [this](const std::vector< uint8_t >& message){
                    NetworkConnectionMessageReceived(message);
                },
                [this](bool graceful){
                    NetworkConnectionBroken(graceful);
                }
            );
        }

        /**
         * This is the type of callback function to be called whenever
         * a new datagram-oriented message is received by the network endpoint.
         *
         * @param[in] address
         *     This is the IPv4 address of the client who sent the message.
         *
         * @param[in] port
         *     This is the port number of the client who sent the message.
         *
         * @param[in] body
         *     This is the contents of the datagram sent by the client.
         */
        void NetworkConnectionPacketReceived(
            uint32_t address,
            uint16_t port,
            const std::vector< uint8_t >& body
        ) {
            std::unique_lock< decltype(mutex) > lock(mutex);
            packetsReceived.emplace_back(body, address, port);
            condition.notify_all();
        }

        /**
         * This is the callback issued whenever more data
         * is received from the peer of the connection.
         *
         * @param[in] message
         *     This contains the data received from
         *     the peer of the connection.
         */
        void NetworkConnectionMessageReceived(const std::vector< uint8_t >& message) {
            std::unique_lock< decltype(mutex) > lock(mutex);
            streamReceived.insert(
                streamReceived.end(),
                message.begin(),
                message.end()
            );
            condition.notify_all();
        }

        /**
         * This is the callback issued whenever
         * the connection is broken.
         *
         * @param[in] graceful
         *     This indicates whether or not the peer of connection
         *     has closed the connection gracefully (meaning we can
         *     continue to send our data back to the peer).
         */
        void NetworkConnectionBroken(bool graceful) {
            if (connectionBrokenDelegate != nullptr) {
                connectionBrokenDelegate(graceful);
            }
            {
                std::unique_lock< decltype(mutex) > lock(mutex);
                connectionBroken = true;
                connectionBrokenGracefully = graceful;
                condition.notify_all();
            }
        }
    };

}

/**
 * This is the test fixture for these tests, providing common
 * setup and teardown for each test.
 */
struct NetworkConnectionTests
    : public ::testing::Test
{
    // Properties

    /**
     * This keeps track of whether or not WSAStartup succeeded,
     * because if so we need to call WSACleanup upon teardown.
     */
    bool wsaStarted = false;

    /**
     * This is the unit under test.
     */
    SystemAbstractions::NetworkConnection client;

    /**
     * This is used to capture callbacks from the unit under test.
     */
    std::shared_ptr< Owner > clientOwner = std::make_shared< Owner >();

    /**
     * These are the diagnostic messages that have been
     * received from the unit under test.
     */
    std::vector< std::string > diagnosticMessages;

    /**
     * This is the delegate obtained when subscribing
     * to receive diagnostic messages from the unit under test.
     * It's called to terminate the subscription.
     */
    SystemAbstractions::DiagnosticsSender::UnsubscribeDelegate diagnosticsUnsubscribeDelegate;

    /**
     * If this flag is set, we will print all received diagnostic
     * messages, in addition to storing them.
     */
    bool printDiagnosticMessages = false;

    // Methods

    // ::testing::Test

    virtual void SetUp() {
#if _WIN32
        WSADATA wsaData;
        if (!WSAStartup(MAKEWORD(2, 0), &wsaData)) {
            wsaStarted = true;
        }
#endif /* _WIN32 */
        diagnosticsUnsubscribeDelegate = client.SubscribeToDiagnostics(
            [this](
                std::string senderName,
                size_t level,
                std::string message
            ){
                diagnosticMessages.push_back(
                    SystemAbstractions::sprintf(
                        "%s[%zu]: %s",
                        senderName.c_str(),
                        level,
                        message.c_str()
                    )
                );
                if (printDiagnosticMessages) {
                    printf(
                        "%s[%zu]: %s\n",
                        senderName.c_str(),
                        level,
                        message.c_str()
                    );
                }
            },
            1
        );
    }

    virtual void TearDown() {
        diagnosticsUnsubscribeDelegate();
#if _WIN32
        if (wsaStarted) {
            (void)WSACleanup();
        }
#endif /* _WIN32 */
    }
};

TEST_F(NetworkConnectionTests, EstablishConnection) {
    SystemAbstractions::NetworkEndpoint server;
    std::vector< std::shared_ptr< SystemAbstractions::NetworkConnection > > clients;
    std::condition_variable_any callbackCondition;
    std::mutex callbackMutex;
    const auto newConnectionDelegate = [&clients, &callbackCondition, &callbackMutex](
        std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
    ){
        std::unique_lock< std::mutex > lock(callbackMutex);
        clients.push_back(newConnection);
        callbackCondition.notify_all();
    };
    const auto packetReceivedDelegate = [](
        uint32_t address,
        uint16_t port,
        const std::vector< uint8_t >& body
    ){
    };
    ASSERT_TRUE(
        server.Open(
            newConnectionDelegate,
            packetReceivedDelegate,
            SystemAbstractions::NetworkEndpoint::Mode::Connection,
            0,
            0,
            0
        )
    );
    ASSERT_FALSE(client.IsConnected());
    ASSERT_TRUE(client.Connect(0x7F000001, server.GetBoundPort()));
    {
        std::unique_lock< decltype(callbackMutex) > lock(callbackMutex);
        ASSERT_TRUE(
            callbackCondition.wait_for(
                lock,
                std::chrono::seconds(1),
                [&clients]{
                    return !clients.empty();
                }
            )
        );
    }
    ASSERT_EQ(server.GetBoundPort(), client.GetPeerPort());
    ASSERT_EQ(0x7F000001, client.GetPeerAddress());
    ASSERT_TRUE(client.IsConnected());
    ASSERT_EQ(client.GetBoundPort(), clients[0]->GetPeerPort());
    ASSERT_EQ(client.GetBoundAddress(), clients[0]->GetPeerAddress());
}

TEST_F(NetworkConnectionTests, SendingMessage) {
    SystemAbstractions::NetworkEndpoint server;
    Owner serverConnectionOwner;
    std::vector< std::shared_ptr< SystemAbstractions::NetworkConnection > > clients;
    std::condition_variable_any callbackCondition;
    std::mutex callbackMutex;
    const auto newConnectionDelegate = [
        &clients,
        &callbackCondition,
        &callbackMutex,
        &serverConnectionOwner
    ](
        std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
    ){
        std::unique_lock< std::mutex > lock(callbackMutex);
        clients.push_back(newConnection);
        ASSERT_TRUE(
            newConnection->Process(
                [&serverConnectionOwner](const std::vector< uint8_t >& message){
                    serverConnectionOwner.NetworkConnectionMessageReceived(message);
                },
                [&serverConnectionOwner](bool graceful){
                    serverConnectionOwner.NetworkConnectionBroken(graceful);
                }
            )
        );
        callbackCondition.notify_all();
    };
    const auto packetReceivedDelegate = [](
        uint32_t address,
        uint16_t port,
        const std::vector< uint8_t >& body
    ){
    };
    ASSERT_TRUE(
        server.Open(
            newConnectionDelegate,
            packetReceivedDelegate,
            SystemAbstractions::NetworkEndpoint::Mode::Connection,
            0,
            0,
            0
        )
    );
    ASSERT_TRUE(client.Connect(0x7F000001, server.GetBoundPort()));
    auto clientOwnerCopy = clientOwner;
    ASSERT_TRUE(
        client.Process(
            [clientOwnerCopy](const std::vector< uint8_t >& message){
                clientOwnerCopy->NetworkConnectionMessageReceived(message);
            },
            [clientOwnerCopy](bool graceful){
                clientOwnerCopy->NetworkConnectionBroken(graceful);
            }
        )
    );
    const std::string messageAsString("Hello, World!");
    const std::vector< uint8_t > messageAsVector(messageAsString.begin(), messageAsString.end());
    client.SendMessage(messageAsVector);
    ASSERT_TRUE(serverConnectionOwner.AwaitStream(messageAsVector.size()));
    ASSERT_EQ(messageAsVector, serverConnectionOwner.streamReceived);
}

TEST_F(NetworkConnectionTests, ReceivingMessage) {
    SystemAbstractions::NetworkEndpoint server;
    Owner serverConnectionOwner;
    std::vector< std::shared_ptr< SystemAbstractions::NetworkConnection > > clients;
    std::condition_variable_any callbackCondition;
    std::mutex callbackMutex;
    const auto newConnectionDelegate = [
        &clients,
        &callbackCondition,
        &callbackMutex,
        &serverConnectionOwner
    ](
        std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
    ){
        std::unique_lock< std::mutex > lock(callbackMutex);
        clients.push_back(newConnection);
        ASSERT_TRUE(
            newConnection->Process(
                [&serverConnectionOwner](const std::vector< uint8_t >& message){
                    serverConnectionOwner.NetworkConnectionMessageReceived(message);
                },
                [&serverConnectionOwner](bool graceful){
                    serverConnectionOwner.NetworkConnectionBroken(graceful);
                }
            )
        );
        callbackCondition.notify_all();
    };
    const auto packetReceivedDelegate = [](
        uint32_t address,
        uint16_t port,
        const std::vector< uint8_t >& body
    ){
    };
    ASSERT_TRUE(
        server.Open(
            newConnectionDelegate,
            packetReceivedDelegate,
            SystemAbstractions::NetworkEndpoint::Mode::Connection,
            0,
            0,
            0
        )
    );
    ASSERT_TRUE(client.Connect(0x7F000001, server.GetBoundPort()));
    auto clientOwnerCopy = clientOwner;
    ASSERT_TRUE(
        client.Process(
            [clientOwnerCopy](const std::vector< uint8_t >& message){
                clientOwnerCopy->NetworkConnectionMessageReceived(message);
            },
            [clientOwnerCopy](bool graceful){
                clientOwnerCopy->NetworkConnectionBroken(graceful);
            }
        )
    );
    const std::string messageAsString("Hello, World!");
    const std::vector< uint8_t > messageAsVector(messageAsString.begin(), messageAsString.end());
    {
        std::unique_lock< decltype(callbackMutex) > lock(callbackMutex);
        ASSERT_TRUE(
            callbackCondition.wait_for(
                lock,
                std::chrono::seconds(1),
                [&clients]{
                    return !clients.empty();
                }
            )
        );
    }
    clients[0]->SendMessage(messageAsVector);
    ASSERT_TRUE(clientOwner->AwaitStream(messageAsVector.size()));
    ASSERT_EQ(messageAsVector, clientOwner->streamReceived);
}

TEST_F(NetworkConnectionTests, Close) {
    SystemAbstractions::NetworkEndpoint server;
    Owner serverConnectionOwner;
    std::vector< std::shared_ptr< SystemAbstractions::NetworkConnection > > clients;
    std::condition_variable_any callbackCondition;
    std::mutex callbackMutex;
    const auto newConnectionDelegate = [
        &clients,
        &callbackCondition,
        &callbackMutex,
        &serverConnectionOwner
    ](
        std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
    ){
        std::unique_lock< std::mutex > lock(callbackMutex);
        clients.push_back(newConnection);
        ASSERT_TRUE(
            newConnection->Process(
                [&serverConnectionOwner](const std::vector< uint8_t >& message){
                    serverConnectionOwner.NetworkConnectionMessageReceived(message);
                },
                [&serverConnectionOwner](bool graceful){
                    serverConnectionOwner.NetworkConnectionBroken(graceful);
                }
            )
        );
        callbackCondition.notify_all();
    };
    const auto packetReceivedDelegate = [](
        uint32_t address,
        uint16_t port,
        const std::vector< uint8_t >& body
    ){
    };
    ASSERT_TRUE(
        server.Open(
            newConnectionDelegate,
            packetReceivedDelegate,
            SystemAbstractions::NetworkEndpoint::Mode::Connection,
            0,
            0,
            0
        )
    );
    ASSERT_TRUE(client.Connect(0x7F000001, server.GetBoundPort()));
    auto clientOwnerCopy = clientOwner;
    ASSERT_TRUE(
        client.Process(
            [clientOwnerCopy](const std::vector< uint8_t >& message){
                clientOwnerCopy->NetworkConnectionMessageReceived(message);
            },
            [clientOwnerCopy](bool graceful){
                clientOwnerCopy->NetworkConnectionBroken(graceful);
            }
        )
    );
    const std::string messageAsString("Hello, World!");
    const std::vector< uint8_t > messageAsVector(messageAsString.begin(), messageAsString.end());
    {
        std::unique_lock< decltype(callbackMutex) > lock(callbackMutex);
        ASSERT_TRUE(
            callbackCondition.wait_for(
                lock,
                std::chrono::seconds(1),
                [&clients]{
                    return !clients.empty();
                }
            )
        );
    }
    ASSERT_FALSE(serverConnectionOwner.connectionBroken);
    client.Close();
    ASSERT_TRUE(serverConnectionOwner.AwaitDisconnection());
    ASSERT_TRUE(serverConnectionOwner.connectionBroken);
}

TEST_F(NetworkConnectionTests, CloseDuringBrokenConnectionCallback) {
    SystemAbstractions::NetworkEndpoint server;
    Owner serverConnectionOwner;
    std::vector< std::shared_ptr< SystemAbstractions::NetworkConnection > > clients;
    std::condition_variable_any callbackCondition;
    std::mutex callbackMutex;
    const auto newConnectionDelegate = [
        &clients,
        &callbackCondition,
        &callbackMutex,
        &serverConnectionOwner
    ](
        std::shared_ptr< SystemAbstractions::NetworkConnection > newConnection
    ){
        std::unique_lock< std::mutex > lock(callbackMutex);
        clients.push_back(newConnection);
        ASSERT_TRUE(
            newConnection->Process(
                [&serverConnectionOwner](const std::vector< uint8_t >& message){
                    serverConnectionOwner.NetworkConnectionMessageReceived(message);
                },
                [&serverConnectionOwner](bool graceful){
                    serverConnectionOwner.NetworkConnectionBroken(graceful);
                }
            )
        );
        callbackCondition.notify_all();
    };
    const auto packetReceivedDelegate = [](
        uint32_t address,
        uint16_t port,
        const std::vector< uint8_t >& body
    ){
    };
    ASSERT_TRUE(
        server.Open(
            newConnectionDelegate,
            packetReceivedDelegate,
            SystemAbstractions::NetworkEndpoint::Mode::Connection,
            0,
            0,
            0
        )
    );
    clientOwner->connectionBrokenDelegate = [this](bool){ client.Close(); };
    (void)client.Connect(0x7F000001, server.GetBoundPort());
    auto clientOwnerCopy = clientOwner;
    (void)client.Process(
        [clientOwnerCopy](const std::vector< uint8_t >& message){
            clientOwnerCopy->NetworkConnectionMessageReceived(message);
        },
        [clientOwnerCopy](bool graceful){
            clientOwnerCopy->NetworkConnectionBroken(graceful);
        }
    );
    {
        std::unique_lock< decltype(callbackMutex) > lock(callbackMutex);
        ASSERT_TRUE(
            callbackCondition.wait_for(
                lock,
                std::chrono::seconds(1),
                [&clients]{
                    return !clients.empty();
                }
            )
        );
    }
    clients[0]->Close();
    clientOwner->AwaitDisconnection();
}

TEST_F(NetworkConnectionTests, InitiateCloseGracefullyWithDataQueued) {
    // Set up a connection-oriented socket to receive
    // a connection from the unit under test.
    auto server = socket(AF_INET, SOCK_STREAM, 0);
    std::shared_ptr< SOCKET > serverReference(
        &server,
        [&server](SOCKET* s){
#if _WIN32
            if (server != INVALID_SOCKET) {
#else /* POSIX */
            if (server >= 0) {
#endif /* _WIN32 or POSIX */
                closesocket(server);
            }
        }
    );
#if _WIN32
    ASSERT_FALSE(server == INVALID_SOCKET);
#else /* POSIX */
    ASSERT_FALSE(server < 0);
#endif /* _WIN32 or POSIX */
    struct sockaddr_in serverAddress;
    (void)memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.IPV4_ADDRESS_IN_SOCKADDR = 0;
    serverAddress.sin_port = 0;
    ASSERT_TRUE(bind(server, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == 0);
    SOCKADDR_LENGTH_TYPE serverAddressLength = sizeof(serverAddress);
    uint16_t serverPort;
    ASSERT_TRUE(getsockname(server, (struct sockaddr*)&serverAddress, &serverAddressLength) == 0);
    serverPort = ntohs(serverAddress.sin_port);
    ASSERT_TRUE(listen(server, SOMAXCONN) == 0);

    // Have unit under test connect to the server.
    SOCKET serverConnection;
    std::shared_ptr< SOCKET > serverConnectionReference(
        &serverConnection,
        [&serverConnection](SOCKET* s){
#if _WIN32
            if (serverConnection != INVALID_SOCKET) {
#else /* POSIX */
            if (serverConnection >= 0) {
#endif /* _WIN32 or POSIX */
                closesocket(serverConnection);
            }
        }
    );
    std::thread serverAccept(
        [server, &serverConnection]{
            struct sockaddr_in clientAddress;
            SOCKADDR_LENGTH_TYPE clientAddressLength = sizeof(clientAddress);
            serverConnection = accept(server, (struct sockaddr*)&clientAddress, &clientAddressLength);
        #if _WIN32
            ASSERT_FALSE(serverConnection == INVALID_SOCKET);
        #else /* POSIX */
            ASSERT_FALSE(serverConnection < 0);
        #endif /* _WIN32 or POSIX */
        }
    );
    ASSERT_TRUE(client.Connect(0x7F000001, serverPort));
    serverAccept.join();
    auto clientOwnerCopy = clientOwner;
    (void)client.Process(
        [clientOwnerCopy](const std::vector< uint8_t >& message){
            clientOwnerCopy->NetworkConnectionMessageReceived(message);
        },
        [clientOwnerCopy](bool graceful){
            clientOwnerCopy->NetworkConnectionBroken(graceful);
        }
    );

    // Queue up to send a large amount of data to the server.
    std::vector< uint8_t > data(10000000, 'X');
    client.SendMessage(data);

    // Issue a graceful close on the client.
    client.Close(true);

    // Verify server connection is not broken until all the
    // data is received.
    std::vector< uint8_t > buffer(100000);
    size_t totalBytesReceived = 0;
    while (totalBytesReceived < data.size()) {
        const auto bytesToReceive = std::min(
            buffer.size(),
            data.size() - totalBytesReceived
        );
        const auto bytesReceived = recv(
            serverConnection,
            (char*) buffer.data(),
            (int)bytesToReceive,
            MSG_WAITALL
        );
        ASSERT_FALSE(bytesReceived <= 0) << totalBytesReceived;
        totalBytesReceived += bytesReceived;
    }

    // Verify unit under test has not yet indicated
    // that the connection is broken.
    ASSERT_FALSE(clientOwner->connectionBroken);

    // Close the server connection and verify the connection
    // is finally broken.
    (void)closesocket(serverConnection);
#if _WIN32
    serverConnection = INVALID_SOCKET;
#else /* POSIX */
    serverConnection = -1;
#endif /* _WIN32 or POSIX */
    ASSERT_TRUE(clientOwner->AwaitDisconnection());
}

TEST_F(NetworkConnectionTests, ReceiveCloseGracefully) {
    // Set up a connection-oriented socket to receive
    // a connection from the unit under test.
    auto server = socket(AF_INET, SOCK_STREAM, 0);
    std::shared_ptr< SOCKET > serverReference(
        &server,
        [&server](SOCKET* s){
#if _WIN32
            if (server != INVALID_SOCKET) {
#else /* POSIX */
            if (server >= 0) {
#endif /* _WIN32 or POSIX */
                closesocket(server);
            }
        }
    );
#if _WIN32
    ASSERT_FALSE(server == INVALID_SOCKET);
#else /* POSIX */
    ASSERT_FALSE(server < 0);
#endif /* _WIN32 or POSIX */
    struct sockaddr_in serverAddress;
    (void)memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.IPV4_ADDRESS_IN_SOCKADDR = 0;
    serverAddress.sin_port = 0;
    ASSERT_TRUE(bind(server, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == 0);
    SOCKADDR_LENGTH_TYPE serverAddressLength = sizeof(serverAddress);
    uint16_t serverPort;
    ASSERT_TRUE(getsockname(server, (struct sockaddr*)&serverAddress, &serverAddressLength) == 0);
    serverPort = ntohs(serverAddress.sin_port);
    ASSERT_TRUE(listen(server, SOMAXCONN) == 0);

    // Have unit under test connect to the server.
    SOCKET serverConnection;
    std::shared_ptr< SOCKET > serverConnectionReference(
        &serverConnection,
        [&serverConnection](SOCKET* s){
#if _WIN32
            if (serverConnection != INVALID_SOCKET) {
#else /* POSIX */
            if (serverConnection >= 0) {
#endif /* _WIN32 or POSIX */
                closesocket(serverConnection);
            }
        }
    );
    std::thread serverAccept(
        [server, &serverConnection]{
            struct sockaddr_in clientAddress;
            SOCKADDR_LENGTH_TYPE clientAddressLength = sizeof(clientAddress);
            serverConnection = accept(server, (struct sockaddr*)&clientAddress, &clientAddressLength);
        #if _WIN32
            ASSERT_FALSE(serverConnection == INVALID_SOCKET);
        #else /* POSIX */
            ASSERT_FALSE(serverConnection < 0);
        #endif /* _WIN32 or POSIX */
        }
    );
    ASSERT_TRUE(client.Connect(0x7F000001, serverPort));
    serverAccept.join();
    auto clientOwnerCopy = clientOwner;
    (void)client.Process(
        [clientOwnerCopy](const std::vector< uint8_t >& message){
            clientOwnerCopy->NetworkConnectionMessageReceived(message);
        },
        [clientOwnerCopy](bool graceful){
            clientOwnerCopy->NetworkConnectionBroken(graceful);
        }
    );

    // Queue up to send a large amount of data to the server.
    std::vector< uint8_t > data(10000000, 'X');
    client.SendMessage(data);

    // Issue a graceful close from the server.
#if _WIN32
    (void)shutdown(serverConnection, SD_SEND);
#else /* POSIX */
    (void)shutdown(serverConnection, SHUT_WR);
#endif /* _WIN32 or POSIX */

    // Verify client receives the graceful close notification.
    EXPECT_TRUE(clientOwner->AwaitDisconnection());
    EXPECT_TRUE(clientOwner->connectionBrokenGracefully);
    clientOwner->connectionBroken = false;
    clientOwner->connectionBrokenGracefully = false;

    // Close the client end of the connection.
    client.Close(true);

    // Verify client connection is not broken until all the
    // data is sent.
    std::vector< uint8_t > buffer(100000);
    size_t totalBytesReceived = 0;
    while (totalBytesReceived < data.size()) {
        const auto bytesToReceive = std::min(
            buffer.size(),
            data.size() - totalBytesReceived
        );
        const auto bytesReceived = recv(
            serverConnection,
            (char*) buffer.data(),
            (int)bytesToReceive,
            MSG_WAITALL
        );
        ASSERT_FALSE(bytesReceived <= 0) << totalBytesReceived << " recv returned: " << bytesReceived;
        totalBytesReceived += bytesReceived;
    }

    // Verify the server closes its end after sending
    // the last message.
    const auto bytesToReceive = buffer.size();
    const auto bytesReceived = recv(
        serverConnection,
        (char*) buffer.data(),
        (int)bytesToReceive,
        MSG_WAITALL
    );
    EXPECT_TRUE(bytesReceived == 0) << bytesReceived;

    // Verify unit under test receives the indication
    // that the connection is broken.
    ASSERT_TRUE(clientOwner->AwaitDisconnection());
    EXPECT_FALSE(clientOwner->connectionBrokenGracefully);
    ASSERT_EQ(
        (std::vector< std::string >{
            "NetworkConnection[1]: connection closed gracefully by peer",
            "NetworkConnection[1]: closing connection",
            "NetworkConnection[1]: closed connection",
        }),
        diagnosticMessages
    );
}

TEST_F(NetworkConnectionTests, InitiateCloseAbruptly) {
    // Set up a connection-oriented socket to receive
    // a connection from the unit under test.
    auto server = socket(AF_INET, SOCK_STREAM, 0);
    std::shared_ptr< SOCKET > serverReference(
        &server,
        [&server](SOCKET* s){
#if _WIN32
            if (server != INVALID_SOCKET) {
#else /* POSIX */
            if (server >= 0) {
#endif /* _WIN32 or POSIX */
                closesocket(server);
            }
        }
    );
#if _WIN32
    ASSERT_FALSE(server == INVALID_SOCKET);
#else /* POSIX */
    ASSERT_FALSE(server < 0);
#endif /* _WIN32 or POSIX */
    struct sockaddr_in serverAddress;
    (void)memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.IPV4_ADDRESS_IN_SOCKADDR = 0;
    serverAddress.sin_port = 0;
    ASSERT_TRUE(bind(server, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == 0);
    SOCKADDR_LENGTH_TYPE serverAddressLength = sizeof(serverAddress);
    uint16_t serverPort;
    ASSERT_TRUE(getsockname(server, (struct sockaddr*)&serverAddress, &serverAddressLength) == 0);
    serverPort = ntohs(serverAddress.sin_port);
    ASSERT_TRUE(listen(server, SOMAXCONN) == 0);

    // Have unit under test connect to the server.
    SOCKET serverConnection;
    std::shared_ptr< SOCKET > serverConnectionReference(
        &serverConnection,
        [&serverConnection](SOCKET* s){
#if _WIN32
            if (serverConnection != INVALID_SOCKET) {
#else /* POSIX */
            if (serverConnection >= 0) {
#endif /* _WIN32 or POSIX */
                closesocket(serverConnection);
            }
        }
    );
    std::thread serverAccept(
        [server, &serverConnection]{
            struct sockaddr_in clientAddress;
            SOCKADDR_LENGTH_TYPE clientAddressLength = sizeof(clientAddress);
            serverConnection = accept(server, (struct sockaddr*)&clientAddress, &clientAddressLength);
        #if _WIN32
            ASSERT_FALSE(serverConnection == INVALID_SOCKET);
        #else /* POSIX */
            ASSERT_FALSE(serverConnection < 0);
        #endif /* _WIN32 or POSIX */
        }
    );
    ASSERT_TRUE(client.Connect(0x7F000001, serverPort));
    serverAccept.join();
    auto clientOwnerCopy = clientOwner;
    (void)client.Process(
        [clientOwnerCopy](const std::vector< uint8_t >& message){
            clientOwnerCopy->NetworkConnectionMessageReceived(message);
        },
        [clientOwnerCopy](bool graceful){
            clientOwnerCopy->NetworkConnectionBroken(graceful);
        }
    );

    // Queue up to send a large amount of data to the server.
    std::vector< uint8_t > data(10000000, 'X');
    client.SendMessage(data);

    // Issue an abrupt close on the client.
    client.Close(false);

    // Verify server connection is broken before all the
    // data is received.
    std::vector< uint8_t > buffer(100000);
    size_t totalBytesReceived = 0;
    while (totalBytesReceived < data.size()) {
        const auto bytesToReceive = std::min(
            buffer.size(),
            data.size() - totalBytesReceived
        );
        const auto bytesReceived = recv(
            serverConnection,
            (char*) buffer.data(),
            (int)bytesToReceive,
            MSG_WAITALL
        );
        EXPECT_FALSE(bytesReceived == 0);
        if (bytesReceived <= 0) {
            break;
        }
        totalBytesReceived += bytesReceived;
    }
    EXPECT_LT(totalBytesReceived, data.size());

    // Verify unit under test has indicated
    // that the connection is broken.
    EXPECT_TRUE(clientOwner->AwaitDisconnection());
}

TEST_F(NetworkConnectionTests, ReceiveCloseAbruptly) {
    // Set up a connection-oriented socket to receive
    // a connection from the unit under test.
    auto server = socket(AF_INET, SOCK_STREAM, 0);
    std::shared_ptr< SOCKET > serverReference(
        &server,
        [&server](SOCKET* s){
#if _WIN32
            if (server != INVALID_SOCKET) {
#else /* POSIX */
            if (server >= 0) {
#endif /* _WIN32 or POSIX */
                closesocket(server);
            }
        }
    );
#if _WIN32
    ASSERT_FALSE(server == INVALID_SOCKET);
#else /* POSIX */
    ASSERT_FALSE(server < 0);
#endif /* _WIN32 or POSIX */
    struct sockaddr_in serverAddress;
    (void)memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.IPV4_ADDRESS_IN_SOCKADDR = 0;
    serverAddress.sin_port = 0;
    ASSERT_TRUE(bind(server, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == 0);
    SOCKADDR_LENGTH_TYPE serverAddressLength = sizeof(serverAddress);
    uint16_t serverPort;
    ASSERT_TRUE(getsockname(server, (struct sockaddr*)&serverAddress, &serverAddressLength) == 0);
    serverPort = ntohs(serverAddress.sin_port);
    ASSERT_TRUE(listen(server, SOMAXCONN) == 0);

    // Have unit under test connect to the server.
    SOCKET serverConnection;
    std::shared_ptr< SOCKET > serverConnectionReference(
        &serverConnection,
        [&serverConnection](SOCKET* s){
#if _WIN32
            if (serverConnection != INVALID_SOCKET) {
#else /* POSIX */
            if (serverConnection >= 0) {
#endif /* _WIN32 or POSIX */
                closesocket(serverConnection);
            }
        }
    );
    std::thread serverAccept(
        [server, &serverConnection]{
            struct sockaddr_in clientAddress;
            SOCKADDR_LENGTH_TYPE clientAddressLength = sizeof(clientAddress);
            serverConnection = accept(server, (struct sockaddr*)&clientAddress, &clientAddressLength);
        #if _WIN32
            ASSERT_FALSE(serverConnection == INVALID_SOCKET);
        #else /* POSIX */
            ASSERT_FALSE(serverConnection < 0);
        #endif /* _WIN32 or POSIX */
        }
    );
    ASSERT_TRUE(client.Connect(0x7F000001, serverPort));
    serverAccept.join();
    auto clientOwnerCopy = clientOwner;
    (void)client.Process(
        [clientOwnerCopy](const std::vector< uint8_t >& message){
            clientOwnerCopy->NetworkConnectionMessageReceived(message);
        },
        [clientOwnerCopy](bool graceful){
            clientOwnerCopy->NetworkConnectionBroken(graceful);
        }
    );

    // Queue up to send a large amount of data to the server.
    std::vector< uint8_t > data(10000000, 'X');
    client.SendMessage(data);

    // Issue an abrupt close from the server.
#ifdef _WIN32
    LINGER linger;
    linger.l_onoff = 1;
    linger.l_linger = 0;
    (void)setsockopt(serverConnection, SOL_SOCKET, SO_LINGER, (const char*)&linger, sizeof(linger));
#else /* POSIX */
    struct linger linger;
    linger.l_onoff = 1;
    linger.l_linger = 0;
    (void)setsockopt(serverConnection, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger));
#endif /* _WIN32 or POSIX */
    (void)closesocket(serverConnection);

    // Verify client receives the abrupt close notification.
    EXPECT_TRUE(clientOwner->AwaitDisconnection());
    EXPECT_FALSE(clientOwner->connectionBrokenGracefully);
    ASSERT_EQ(
        (std::vector< std::string >{
            "NetworkConnection[1]: connection closed abruptly by peer",
            "NetworkConnection[1]: closed connection",
        }),
        diagnosticMessages
    );
}

TEST_F(NetworkConnectionTests, InitiateCloseGracefullyNoDataQueued) {
    // Set up a connection-oriented socket to receive
    // a connection from the unit under test.
    auto server = socket(AF_INET, SOCK_STREAM, 0);
    std::shared_ptr< SOCKET > serverReference(
        &server,
        [&server](SOCKET* s){
#if _WIN32
            if (server != INVALID_SOCKET) {
#else /* POSIX */
            if (server >= 0) {
#endif /* _WIN32 or POSIX */
                closesocket(server);
            }
        }
    );
#if _WIN32
    ASSERT_FALSE(server == INVALID_SOCKET);
#else /* POSIX */
    ASSERT_FALSE(server < 0);
#endif /* _WIN32 or POSIX */
    struct sockaddr_in serverAddress;
    (void)memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.IPV4_ADDRESS_IN_SOCKADDR = 0;
    serverAddress.sin_port = 0;
    ASSERT_TRUE(bind(server, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == 0);
    SOCKADDR_LENGTH_TYPE serverAddressLength = sizeof(serverAddress);
    uint16_t serverPort;
    ASSERT_TRUE(getsockname(server, (struct sockaddr*)&serverAddress, &serverAddressLength) == 0);
    serverPort = ntohs(serverAddress.sin_port);
    ASSERT_TRUE(listen(server, SOMAXCONN) == 0);

    // Have unit under test connect to the server.
    SOCKET serverConnection;
    std::shared_ptr< SOCKET > serverConnectionReference(
        &serverConnection,
        [&serverConnection](SOCKET* s){
#if _WIN32
            if (serverConnection != INVALID_SOCKET) {
#else /* POSIX */
            if (serverConnection >= 0) {
#endif /* _WIN32 or POSIX */
                closesocket(serverConnection);
            }
        }
    );
    std::thread serverAccept(
        [server, &serverConnection]{
            struct sockaddr_in clientAddress;
            SOCKADDR_LENGTH_TYPE clientAddressLength = sizeof(clientAddress);
            serverConnection = accept(server, (struct sockaddr*)&clientAddress, &clientAddressLength);
        #if _WIN32
            ASSERT_FALSE(serverConnection == INVALID_SOCKET);
        #else /* POSIX */
            ASSERT_FALSE(serverConnection < 0);
        #endif /* _WIN32 or POSIX */
        }
    );
    ASSERT_TRUE(client.Connect(0x7F000001, serverPort));
    serverAccept.join();
    auto clientOwnerCopy = clientOwner;
    (void)client.Process(
        [clientOwnerCopy](const std::vector< uint8_t >& message){
            clientOwnerCopy->NetworkConnectionMessageReceived(message);
        },
        [clientOwnerCopy](bool graceful){
            clientOwnerCopy->NetworkConnectionBroken(graceful);
        }
    );

    // Issue a graceful close on the client.
    client.Close(true);

    // Verify that the server side detects the graceful close.
#if _WIN32
    u_long blockingMode = 1;
    (void)ioctlsocket(serverConnection, FIONBIO, &blockingMode);
#else /* POSIX */
    int flags = fcntl(serverConnection, F_GETFL, 0);
    flags |= O_NONBLOCK;
    (void)fcntl(serverConnection, F_SETFL, flags);
#endif /* _WIN32 or POSIX */
    bool wouldBlock = true;
    const auto startTime = time(NULL);
    while (wouldBlock) {
        wouldBlock = false;
        ASSERT_FALSE(time(NULL) - startTime > 1);
        std::vector< uint8_t > buffer(100000);
        const auto bytesReceived = recv(
            serverConnection,
            (char*)buffer.data(),
            (int)buffer.size(),
            0
        );
#if _WIN32
        if (bytesReceived == SOCKET_ERROR) {
            const auto wsaLastError = WSAGetLastError();
            if (wsaLastError == WSAEWOULDBLOCK) {
                wouldBlock = true;
            }
        }
#else /* POSIX */
        if (bytesReceived < 0) {
            if (errno == EWOULDBLOCK) {
                wouldBlock = true;
            }
        }
#endif /* _WIN32 or POSIX */
    }

    // Verify unit under test has not yet indicated
    // that the connection is broken.
    ASSERT_FALSE(clientOwner->connectionBroken);

    // Close the server connection and verify the connection
    // is finally broken.
    (void)closesocket(serverConnection);
#if _WIN32
    serverConnection = INVALID_SOCKET;
#else /* POSIX */
    serverConnection = -1;
#endif /* _WIN32 or POSIX */
    ASSERT_TRUE(clientOwner->AwaitDisconnection());
}

TEST_F(NetworkConnectionTests, GetAddressOfHost) {
    EXPECT_EQ(0x7f000001, SystemAbstractions::NetworkConnection::GetAddressOfHost("localhost"));
    EXPECT_EQ(0x7f000001, SystemAbstractions::NetworkConnection::GetAddressOfHost("127.0.0.1"));
    EXPECT_EQ(0x08080808, SystemAbstractions::NetworkConnection::GetAddressOfHost("8.8.8.8"));
    EXPECT_EQ(0, SystemAbstractions::NetworkConnection::GetAddressOfHost(".example"));
}
