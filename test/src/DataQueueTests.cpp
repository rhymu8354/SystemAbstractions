/**
 * @file DataQueueTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::DataQueue class.
 *
 * © 2018 by Richard Walters
 */

#include <DataQueue.hpp>
#include <gtest/gtest.h>
#include <stdint.h>
#include <vector>

TEST(DataQueueTests, EnqueueCopy) {
    // Arrange
    SystemAbstractions::DataQueue q;
    std::vector< uint8_t > data(10000, 'X');

    // Act
    q.Enqueue(data);
    data.assign(5000, 'Y');
    q.Enqueue(data);

    // Assert
    EXPECT_EQ(2, q.GetBuffersQueued());
    EXPECT_EQ(15000, q.GetBytesQueued());
}

TEST(DataQueueTests, EnqueueMove) {
    // Arrange
    SystemAbstractions::DataQueue q;
    std::vector< uint8_t > data(10000, 'X');

    // Act
    q.Enqueue(std::move(data));
    data.assign(5000, 'Y');
    q.Enqueue(std::move(data));

    // Assert
    EXPECT_EQ(2, q.GetBuffersQueued());
    EXPECT_EQ(15000, q.GetBytesQueued());
}

TEST(DataQueueTests, DequeuePartialBuffer) {
    // Arrange
    SystemAbstractions::DataQueue q;
    std::vector< uint8_t > data(10, 'X');
    q.Enqueue(data);
    data.assign(5, 'Y');
    q.Enqueue(data);

    // Act
    data = q.Dequeue(8);

    // Assert
    EXPECT_EQ(
        "XXXXXXXX",
        std::string(data.begin(), data.end())
    );
    EXPECT_EQ(2, q.GetBuffersQueued());
    EXPECT_EQ(7, q.GetBytesQueued());
}

TEST(DataQueueTests, DequeueExactlyOneFullBuffer) {
    // Arrange
    SystemAbstractions::DataQueue q;
    std::vector< uint8_t > data(10, 'X');
    q.Enqueue(data);
    data.assign(5, 'Y');
    q.Enqueue(data);

    // Act
    data = q.Dequeue(10);

    // Assert
    EXPECT_EQ(
        "XXXXXXXXXX",
        std::string(data.begin(), data.end())
    );
    EXPECT_EQ(1, q.GetBuffersQueued());
    EXPECT_EQ(5, q.GetBytesQueued());
}

TEST(DataQueueTests, DequeueFullBufferPlusPartialNextBuffer) {
    // Arrange
    SystemAbstractions::DataQueue q;
    std::vector< uint8_t > data(10, 'X');
    q.Enqueue(data);
    data.assign(5, 'Y');
    q.Enqueue(data);

    // Act
    data = q.Dequeue(12);

    // Assert
    EXPECT_EQ(
        "XXXXXXXXXXYY",
        std::string(data.begin(), data.end())
    );
    EXPECT_EQ(1, q.GetBuffersQueued());
    EXPECT_EQ(3, q.GetBytesQueued());
}

TEST(DataQueueTests, DequeueExactlyOneFullBufferThenTheRest) {
    // Arrange
    SystemAbstractions::DataQueue q;
    std::vector< uint8_t > data(10, 'X');
    q.Enqueue(data);
    data.assign(5, 'Y');
    q.Enqueue(data);
    data = q.Dequeue(10);

    // Act
    data = q.Dequeue(5);

    // Assert
    EXPECT_EQ(
        "YYYYY",
        std::string(data.begin(), data.end())
    );
    EXPECT_EQ(0, q.GetBuffersQueued());
    EXPECT_EQ(0, q.GetBytesQueued());
}

TEST(DataQueueTests, DequeueFullBufferPlusPartialNextBufferThenTheRest) {
    // Arrange
    SystemAbstractions::DataQueue q;
    std::vector< uint8_t > data(10, 'X');
    q.Enqueue(data);
    data.assign(5, 'Y');
    q.Enqueue(data);
    data = q.Dequeue(12);

    // Act
    data = q.Dequeue(3);

    // Assert
    EXPECT_EQ(
        "YYY",
        std::string(data.begin(), data.end())
    );
    EXPECT_EQ(0, q.GetBuffersQueued());
    EXPECT_EQ(0, q.GetBytesQueued());
}
