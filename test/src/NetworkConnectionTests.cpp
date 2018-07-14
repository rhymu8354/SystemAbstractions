/**
 * @file NetworkConnectionTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::NetworkConnection class.
 *
 * © 2018 by Richard Walters
 */

#include <condition_variable>
#include <gtest/gtest.h>
#include <mutex>
#include <stdint.h>
#include <string>
#include <SystemAbstractions/NetworkConnection.hpp>
#include <SystemAbstractions/NetworkEndpoint.hpp>
#include <vector>

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
                [this]{
                    NetworkConnectionBroken();
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
         */
        void NetworkConnectionBroken() {
            std::unique_lock< decltype(mutex) > lock(mutex);
            connectionBroken = true;
            condition.notify_all();
        }
    };

}

TEST(NetworkConnectionTests, EstablishConnection) {
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
    SystemAbstractions::NetworkConnection client;
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
}

TEST(NetworkConnectionTests, SendingMessage) {
    SystemAbstractions::NetworkEndpoint server;
    Owner serverConnectionOwner, clientConnectionOwner;
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
                [&serverConnectionOwner]{
                    serverConnectionOwner.NetworkConnectionBroken();
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
    SystemAbstractions::NetworkConnection client;
    ASSERT_TRUE(client.Connect(0x7F000001, server.GetBoundPort()));
    ASSERT_TRUE(
        client.Process(
            [&clientConnectionOwner](const std::vector< uint8_t >& message){
                clientConnectionOwner.NetworkConnectionMessageReceived(message);
            },
            [&clientConnectionOwner]{
                clientConnectionOwner.NetworkConnectionBroken();
            }
        )
    );
    const std::string messageAsString("Hello, World!");
    const std::vector< uint8_t > messageAsVector(messageAsString.begin(), messageAsString.end());
    client.SendMessage(messageAsVector);
    ASSERT_TRUE(serverConnectionOwner.AwaitStream(messageAsVector.size()));
    ASSERT_EQ(messageAsVector, serverConnectionOwner.streamReceived);
}

TEST(NetworkConnectionTests, ReceivingMessage) {
    SystemAbstractions::NetworkEndpoint server;
    Owner serverConnectionOwner, clientConnectionOwner;
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
                [&serverConnectionOwner]{
                    serverConnectionOwner.NetworkConnectionBroken();
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
    SystemAbstractions::NetworkConnection client;
    ASSERT_TRUE(client.Connect(0x7F000001, server.GetBoundPort()));
    ASSERT_TRUE(
        client.Process(
            [&clientConnectionOwner](const std::vector< uint8_t >& message){
                clientConnectionOwner.NetworkConnectionMessageReceived(message);
            },
            [&clientConnectionOwner]{
                clientConnectionOwner.NetworkConnectionBroken();
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
    ASSERT_TRUE(clientConnectionOwner.AwaitStream(messageAsVector.size()));
    ASSERT_EQ(messageAsVector, clientConnectionOwner.streamReceived);
}

TEST(NetworkConnectionTests, Close) {
    SystemAbstractions::NetworkEndpoint server;
    Owner serverConnectionOwner, clientConnectionOwner;
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
                [&serverConnectionOwner]{
                    serverConnectionOwner.NetworkConnectionBroken();
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
    SystemAbstractions::NetworkConnection client;
    ASSERT_TRUE(client.Connect(0x7F000001, server.GetBoundPort()));
    ASSERT_TRUE(
        client.Process(
            [&clientConnectionOwner](const std::vector< uint8_t >& message){
                clientConnectionOwner.NetworkConnectionMessageReceived(message);
            },
            [&clientConnectionOwner]{
                clientConnectionOwner.NetworkConnectionBroken();
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
