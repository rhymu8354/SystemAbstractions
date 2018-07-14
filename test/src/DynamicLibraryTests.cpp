/**
 * @file DynamicLibraryTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::DynamicLibrary class.
 *
 * © 2018 by Richard Walters
 */

#include <gtest/gtest.h>
#include <SystemAbstractions/DynamicLibrary.hpp>
#include <SystemAbstractions/File.hpp>

TEST(DynamicLibraryTests, LoadAndGetProcedure) {
    SystemAbstractions::DynamicLibrary lib;
    ASSERT_TRUE(
        lib.Load(
            SystemAbstractions::File::GetExeParentDirectory(),
            "MockDynamicLibrary"
        )
    );
    auto procedureAddress = lib.GetProcedure("Foo");
    ASSERT_FALSE(procedureAddress == nullptr);
    int(*procedure)(int) = (int(*)(int))procedureAddress;
    ASSERT_EQ(49, procedure(7));
}

TEST(DynamicLibraryTests, Unload) {
    SystemAbstractions::DynamicLibrary lib;
    ASSERT_TRUE(
        lib.Load(
            SystemAbstractions::File::GetExeParentDirectory(),
            "MockDynamicLibrary"
        )
    );
    auto procedureAddress = lib.GetProcedure("Foo");
    ASSERT_FALSE(procedureAddress == nullptr);
    int(*procedure)(int) = (int(*)(int))procedureAddress;
    lib.Unload();
    ASSERT_DEATH(procedure(7), "");
}
