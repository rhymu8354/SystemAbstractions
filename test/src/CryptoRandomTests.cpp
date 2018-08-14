/**
 * @file CryptoRandomTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::CryptoRandom class.
 *
 * © 2018 by Richard Walters
 */

#include <algorithm>
#include <gtest/gtest.h>
#include <stdlib.h>
#include <SystemAbstractions/CryptoRandom.hpp>
#include <vector>

TEST(CryptoRandomTests, Generate) {
    SystemAbstractions::CryptoRandom generator;
    std::vector< int > counts(256);
    constexpr size_t iterations = 1000000;
    for (size_t i = 0; i < iterations; ++i) {
        uint8_t buffer;
        generator.Generate(&buffer, 1);
        ++counts[buffer];
    }
    int sum = 0;
    for (auto count: counts) {
        sum += count;
    }
    const int average = sum / 256;
    EXPECT_NEAR(iterations / 256, average, 10);
    int largestDifference = 0;
    for (auto count: counts) {
        const auto difference = abs(count - average);
        largestDifference = std::max(largestDifference, difference);
        EXPECT_LE(difference, average / 10);
    }
    printf("Average: %d, Largest difference: %d\n", average, largestDifference);
}
