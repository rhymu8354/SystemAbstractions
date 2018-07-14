/**
 * @file DiagnosticsContextTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::DiagnosticsContext class.
 *
 * © 2018 by Richard Walters
 */

#include <gtest/gtest.h>
#include <SystemAbstractions/DiagnosticsContext.hpp>
#include <SystemAbstractions/DiagnosticsSender.hpp>
#include <vector>

/**
 * This is used to store a message received
 * from a diagnostics sender.
 */
struct ReceivedMessage {
    // Properties

    /**
     * This identifies the origin of the diagnostic information.
     */
    std::string senderName;

    /**
     * This is used to filter out less-important information.
     * The level is higher the more important the information is.
     */
    size_t level;

    /**
     * This is the content of the message.
     */
    std::string message;

    // Methods

    /**
     * This constructor is used to initialize all the fields
     * of the structure.
     *
     * @param[in] newSenderName
     *     This identifies the origin of the diagnostic information.
     *
     * @param[in] newLevel
     *     This is used to filter out less-important information.
     *     The level is higher the more important the information is.
     *
     * @param[in] newMessage
     *     This is the content of the message.
     */
    ReceivedMessage(
        std::string newSenderName,
        size_t newLevel,
        std::string newMessage
    )
        : senderName(newSenderName)
        , level(newLevel)
        , message(newMessage)
    {
    }

    /**
     * This is the equality operator for the class.
     *
     * @param[in] other
     *     This is the other instance to which to compare this instance.
     *
     * @return
     *     An indication of whether or not the two instances are equal
     *     is returned.
     */
    bool operator==(const ReceivedMessage& other) const noexcept {
        return (
            (senderName == other.senderName)
            && (level == other.level)
            && (message == other.message)
        );
    };

};

TEST(DiagnosticsContextTests, PushAndPopContext) {
    SystemAbstractions::DiagnosticsSender sender("foo");
    std::vector< ReceivedMessage > receivedMessages;
    sender.SubscribeToDiagnostics(
        [&receivedMessages](
            std::string senderName,
            size_t level,
            std::string message
        ){
            receivedMessages.emplace_back(
                senderName,
                level,
                message
            );
        }
    );
    sender.SendDiagnosticInformationString(0, "hello");
    {
        SystemAbstractions::DiagnosticsContext testContext(sender, "bar");
        sender.SendDiagnosticInformationString(0, "world");
    }
    sender.SendDiagnosticInformationString(0, "last message");
    ASSERT_EQ(
        (std::vector< ReceivedMessage >{
            { "foo", 0, "hello" },
            { "foo", 0, "bar: world" },
            { "foo", 0, "last message" },
        }),
        receivedMessages
    );
}
