/**
 * @file DiagnosticsSenderTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::DiagnosticsSender class.
 *
 * © 2018 by Richard Walters
 */

#include <gtest/gtest.h>
#include <string>
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

TEST(DiagnosticsSenderTests, BasicSubscriptionAndTransmission) {
    SystemAbstractions::DiagnosticsSender sender("Joe");
    sender.SendDiagnosticInformationString(100, "Very important message nobody will hear; FeelsBadMan");
    std::vector< ReceivedMessage > receivedMessages;
    const auto unsubscribeDelegate = sender.SubscribeToDiagnostics(
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
        },
        5
    );
    ASSERT_EQ(5, sender.GetMinLevel());
    ASSERT_TRUE(unsubscribeDelegate != nullptr);
    sender.SendDiagnosticInformationString(10, "PogChamp");
    sender.SendDiagnosticInformationString(3, "Did you hear that?");
    sender.PushContext("spam");
    sender.SendDiagnosticInformationString(4, "Level 4 whisper...");
    sender.SendDiagnosticInformationString(5, "Level 5, can you dig it?");
    sender.PopContext();
    sender.SendDiagnosticInformationString(6, "Level 6 FOR THE WIN");
    unsubscribeDelegate();
    sender.SendDiagnosticInformationString(5, "Are you still there?");
    ASSERT_EQ(
        receivedMessages,
        (std::vector< ReceivedMessage >{
            { "Joe", 10, "PogChamp" },
            { "Joe", 5, "spam: Level 5, can you dig it?" },
            { "Joe", 6, "Level 6 FOR THE WIN" },
        })
    );
}

TEST(DiagnosticsSenderTests, FormattedMessage) {
    SystemAbstractions::DiagnosticsSender sender("Joe");
    std::vector< ReceivedMessage > receivedMessages;
    (void)sender.SubscribeToDiagnostics(
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
    sender.SendDiagnosticInformationFormatted(0, "The answer is %d.", 42);
    ASSERT_EQ(
        receivedMessages,
        (std::vector< ReceivedMessage >{
            { "Joe", 0, "The answer is 42." },
        })
    );
}

TEST(DiagnosticsSenderTests, Chaining) {
    SystemAbstractions::DiagnosticsSender outer("outer");
    SystemAbstractions::DiagnosticsSender inner("inner");
    std::vector< ReceivedMessage > receivedMessages;
    (void)outer.SubscribeToDiagnostics(
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
    (void)inner.SubscribeToDiagnostics(outer.Chain());
    inner.SendDiagnosticInformationFormatted(0, "The answer is %d.", 42);
    ASSERT_EQ(
        receivedMessages,
        (std::vector< ReceivedMessage >{
            { "outer", 0, "inner: The answer is 42." },
        })
    );
}

TEST(DiagnosticsSenderTests, UnsubscribeAfterSenderDestroyed) {
    std::unique_ptr< SystemAbstractions::DiagnosticsSender > sender(new SystemAbstractions::DiagnosticsSender("sender"));
    std::vector< ReceivedMessage > receivedMessages;
    const auto unsubscribe = sender->SubscribeToDiagnostics(
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
    sender.reset();
    unsubscribe();
}

TEST(DiagnosticsSenderTests, PublishAfterChainedSenderDestroyed) {
    std::unique_ptr< SystemAbstractions::DiagnosticsSender > outer(new SystemAbstractions::DiagnosticsSender("outer"));
    SystemAbstractions::DiagnosticsSender inner("inner");
    std::vector< ReceivedMessage > receivedMessages;
    (void)outer->SubscribeToDiagnostics(
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
    (void)inner.SubscribeToDiagnostics(outer->Chain());
    outer.reset();
    inner.SendDiagnosticInformationFormatted(0, "The answer is %d.", 42);
    ASSERT_EQ(
        receivedMessages,
        (std::vector< ReceivedMessage >{
        })
    );
}
