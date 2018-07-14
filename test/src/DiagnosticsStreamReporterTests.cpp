/**
 * @file DiagnosticsStreamReporterTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::DiagnosticsStreamReporter function object generator.
 *
 * © 2018 by Richard Walters
 */

#include <gtest/gtest.h>
#include <stdio.h>
#include <string>
#include <SystemAbstractions/DiagnosticsStreamReporter.hpp>
#include <SystemAbstractions/DiagnosticsSender.hpp>
#include <SystemAbstractions/File.hpp>
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

/**
 * This is a helper function for the unit tests, that reads the
 * next line of the given file, and checks to make sure that
 * the line begins correctly, and after the timestamp,
 * patches the given expected string.
 *
 * @param[in] f
 *     This is the log file.
 *
 * @param[in] expected
 *     This is what we expect to be on the next line, after
 *     the timestamp part of the log message.
 */
void CheckLogMessage(
    FILE* f,
    const std::string& expected
) {
    std::vector< char > lineBuffer(256);
    const auto lineCString = fgets(lineBuffer.data(), (int)lineBuffer.size(), f);
    ASSERT_FALSE(lineCString == NULL);
    const std::string lineCppString(lineCString);
    ASSERT_GE(lineCppString.length(), 2);
    ASSERT_EQ('[', lineCppString[0]);
    const auto firstSpace = lineCppString.find(' ');
    ASSERT_FALSE(firstSpace == std::string::npos);
    ASSERT_EQ(expected, lineCppString.substr(firstSpace + 1));
}

/**
 * This is a helper function for the unit tests, that checks to
 * make sure we are at the end of the given log file.
 *
 * @param[in] f
 *     This is the log file.
 */
void CheckIsEndOfFile(FILE* f) {
    ASSERT_EQ(EOF, fgetc(f));
}

/**
 * This is the test fixture for these tests, providing common
 * setup and teardown for each test.
 */
struct DiagnosticsStreamReporterTests
    : public ::testing::Test
{
    // Properties

    /**
     * This is the temporary directory to use to test
     * the File class.
     */
    std::string testAreaPath;

    // Methods

    // ::testing::Test

    virtual void SetUp() {
        testAreaPath = SystemAbstractions::File::GetExeParentDirectory() + "/TestArea";
        ASSERT_TRUE(SystemAbstractions::File::CreateDirectory(testAreaPath));
    }

    virtual void TearDown() {
        ASSERT_TRUE(SystemAbstractions::File::DeleteDirectory(testAreaPath));
    }
};

TEST_F(DiagnosticsStreamReporterTests, SaveDiagnosticMessagesToLogFiles) {
    SystemAbstractions::DiagnosticsSender sender("foo");
    auto output = fopen((testAreaPath + "/out.txt").c_str(), "wt");
    auto error = fopen((testAreaPath + "/error.txt").c_str(), "wt");
    const auto subscriptionToken = sender.SubscribeToDiagnostics(
        SystemAbstractions::DiagnosticsStreamReporter(output, error)
    );
    sender.SendDiagnosticInformationString(0, "hello");
    sender.SendDiagnosticInformationString(10, "world");
    sender.SendDiagnosticInformationString(2, "last message");
    sender.SendDiagnosticInformationString(5, "be careful");
    sender.UnsubscribeFromDiagnostics(subscriptionToken);
    sender.SendDiagnosticInformationString(0, "really the last message");
    (void)fclose(output);
    (void)fclose(error);
    output = fopen((testAreaPath + "/out.txt").c_str(), "rt");
    CheckLogMessage(output, "foo:0] hello\n");
    CheckLogMessage(output, "foo:2] last message\n");
    CheckIsEndOfFile(output);
    (void)fclose(output);
    error = fopen((testAreaPath + "/error.txt").c_str(), "rt");
    CheckLogMessage(error, "foo:10] error: world\n");
    CheckLogMessage(error, "foo:5] warning: be careful\n");
    CheckIsEndOfFile(error);
    (void)fclose(error);
}
