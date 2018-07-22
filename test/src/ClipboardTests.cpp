/**
 * @file ClipboardTests.cpp
 *
 * This module contains the unit tests of the
 * SystemAbstractions::Clipboard class.
 *
 * © 2018 by Richard Walters
 */

#include <gtest/gtest.h>
#include <string>
#include <string.h>
#include <SystemAbstractions/Clipboard.hpp>
#include <vector>

#ifdef _WIN32

/**
 * This is used to redirect the unit under test to talk
 * to a mock operating system interface to get to the clipboard.
 */
struct MockClipboardOperatingSystemInterface
    : public ClipboardOperatingSystemInterface
{
    // Properties

    /**
     * This holds onto whatever was last copied into the clipboard.
     */
    std::string contents;

    /**
     * This indicates whether or not the clipboard contains text.
     */
    bool hasText = false;

    /**
     * This captures what operating system calls were made, in what order.
     */
    std::vector< std::string > callsInOrder;

    /**
     * This holds the last global allocation made by this mock.
     */
    HGLOBAL dataHandle = NULL;

    // Lifecycle Management
    ~MockClipboardOperatingSystemInterface() noexcept {
        if (dataHandle != NULL) {
            GlobalFree(dataHandle);
        }
    }
    MockClipboardOperatingSystemInterface(const MockClipboardOperatingSystemInterface& other) noexcept
        : contents(other.contents)
        , hasText(other.hasText)
        , callsInOrder(other.callsInOrder)
        , dataHandle(NULL)
    {
    }
    MockClipboardOperatingSystemInterface(MockClipboardOperatingSystemInterface&& other) noexcept
        : contents(std::move(other.contents))
        , hasText(other.hasText)
        , callsInOrder(std::move(other.callsInOrder))
        , dataHandle(other.dataHandle)
    {
        other.dataHandle = NULL;
    }
    MockClipboardOperatingSystemInterface& operator=(const MockClipboardOperatingSystemInterface& other) noexcept {
        if (this != &other) {
            if (dataHandle != NULL) {
                GlobalFree(dataHandle);
            }
            contents = other.contents;
            hasText = other.hasText;
            callsInOrder = other.callsInOrder;
            dataHandle = NULL;
        }
        return *this;
    }
    MockClipboardOperatingSystemInterface& operator=(MockClipboardOperatingSystemInterface&& other) noexcept {
        if (this != &other) {
            if (dataHandle != NULL) {
                GlobalFree(dataHandle);
            }
            contents = std::move(other.contents);
            hasText = other.hasText;
            callsInOrder = std::move(other.callsInOrder);
            dataHandle = other.dataHandle;
            other.dataHandle = NULL;
        }
        return *this;
    }

    // Methods

    /**
     * This is the constructor of the class.
     */
    MockClipboardOperatingSystemInterface() = default;

    // ClipboardOperatingSystemInterface
public:
    virtual BOOL OpenClipboard(HWND hWndNewOwner) override {
        callsInOrder.push_back("open");
        return TRUE;
    }

    virtual BOOL EmptyClipboard() override {
        callsInOrder.push_back("empty");
        contents.clear();
        hasText = false;
        return TRUE;
    }

    virtual BOOL IsClipboardFormatAvailable(UINT format) override {
        callsInOrder.push_back("format");
        if (hasText) {
            return (format == CF_TEXT);
        } else {
            return false;
        }
    }

    virtual HANDLE GetClipboardData(UINT uFormat) override {
        callsInOrder.push_back("get");
        if (dataHandle != NULL) {
            GlobalFree(dataHandle);
        }
        dataHandle = NULL;
        if (hasText) {
            if (uFormat == CF_TEXT) {
                dataHandle = GlobalAlloc(GMEM_MOVEABLE, contents.length() + 1);
                if (dataHandle != NULL) {
                    LPSTR data = (LPSTR)GlobalLock(dataHandle);
                    if (data == NULL) {
                        return NULL;
                    } else {
                        (void)memcpy(data, contents.c_str(), contents.length() + 1);
                        (void)GlobalUnlock(data);
                    }
                }
                return dataHandle;
            }
        }
        return NULL;
    }

    virtual void SetClipboardData(
        UINT uFormat,
        HANDLE hMem
    ) override {
        callsInOrder.push_back("set");
        LPSTR data = (LPSTR)GlobalLock(hMem);
        contents = data;
        (void)GlobalUnlock(data);
    }

    virtual BOOL CloseClipboard() override {
        callsInOrder.push_back("close");
        return TRUE;
    }
};

#elif defined(APPLE)

#else /* POSIX */

/**
 * This is used to redirect the unit under test to talk
 * to a mock operating system interface to get to the clipboard.
 */
struct MockClipboardOperatingSystemInterface
    : public ClipboardOperatingSystemInterface
{
    // Properties

    /**
     * This holds onto whatever was last copied into the clipboard.
     */
    std::string contents;

    /**
     * This indicates whether or not the clipboard contains text.
     */
    bool hasText = false;

    /**
     * This captures what operating system calls were made, in what order.
     */
    std::vector< std::string > callsInOrder;

    // Methods

    /**
     * This is the constructor of the class.
     */
    MockClipboardOperatingSystemInterface() = default;

    // ClipboardOperatingSystemInterface
public:
    virtual void Copy(const std::string& s) override {
        callsInOrder.push_back("Copy");
        contents = s;
        hasText = true;
    }

    virtual bool HasString() override {
        callsInOrder.push_back("HasString");
        return hasText;
    }

    virtual std::string PasteString() override {
        callsInOrder.push_back("PasteString");
        if (hasText) {
            return contents;
        } else {
            return "";
        }
    }
};

#endif /* different platforms */

/**
 * This is the test fixture for these tests, providing common
 * setup and teardown for each test.
 */
struct ClipboardTests
    : public ::testing::Test
{
    // Properties

    /**
     * This is used to intercept operating system calls
     * in order to test that they are made.
     */
    MockClipboardOperatingSystemInterface mockClipboard;

    /**
     * This is a copy made of selectedClipboardOperatingSystemInterface
     * before it's changed by the fixture.  It's used to restore
     * the pointer upon tear down.
     */
    ClipboardOperatingSystemInterface* originalSelectedClipboardOperatingSystemInterface;

    // Methods

    // ::testing::Test

    virtual void SetUp() {
        originalSelectedClipboardOperatingSystemInterface = selectedClipboardOperatingSystemInterface;
        selectedClipboardOperatingSystemInterface = &mockClipboard;
    }

    virtual void TearDown() {
        selectedClipboardOperatingSystemInterface = originalSelectedClipboardOperatingSystemInterface;
    }
};

TEST_F(ClipboardTests, Copy) {
    SystemAbstractions::Clipboard clipboard;
    const std::string testString = "Hello, World!";
    clipboard.Copy(testString);
    ASSERT_EQ(testString, mockClipboard.contents);
#ifdef _WIN32
    ASSERT_EQ(
        (std::vector< std::string >{
            "open",
            "empty",
            "set",
            "close",
        }),
        mockClipboard.callsInOrder
    );
#elif defined(APPLE)
    ASSERT_TRUE(false) << "test does not work with APPLE yet";
#else /* Linux */
    ASSERT_EQ(
        (std::vector< std::string >{
            "Copy",
        }),
        mockClipboard.callsInOrder
    );
#endif /* different platforms */
}

TEST_F(ClipboardTests, Paste) {
    SystemAbstractions::Clipboard clipboard;
    const std::string testString = "Hello, World!";
    mockClipboard.contents = testString;
    mockClipboard.hasText = true;
    ASSERT_EQ(testString, clipboard.PasteString());
#ifdef _WIN32
    ASSERT_EQ(
        (std::vector< std::string >{
            "open",
            "format",
            "get",
            "close",
        }),
        mockClipboard.callsInOrder
    );
#elif defined(APPLE)
    ASSERT_TRUE(false) << "test does not work with APPLE yet";
#else /* Linux */
    ASSERT_EQ(
        (std::vector< std::string >{
            "PasteString",
        }),
        mockClipboard.callsInOrder
    );
#endif /* different platforms */
}

TEST_F(ClipboardTests, HasString) {
    SystemAbstractions::Clipboard clipboard;
    const std::string testString = "Hello, World!";
    ASSERT_FALSE(clipboard.HasString());
    mockClipboard.contents = testString;
    mockClipboard.hasText = true;
    ASSERT_TRUE(clipboard.HasString());
#ifdef _WIN32
    ASSERT_EQ(
        (std::vector< std::string >{
            "open",
            "format",
            "close",
            "open",
            "format",
            "close",
        }),
        mockClipboard.callsInOrder
    );
#elif defined(APPLE)
    ASSERT_TRUE(false) << "test does not work with APPLE yet";
#else /* Linux */
    ASSERT_EQ(
        (std::vector< std::string >{
            "HasString",
            "HasString",
        }),
        mockClipboard.callsInOrder
    );
#endif /* different platforms */
}
