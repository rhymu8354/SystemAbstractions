/**
 * @file ClipboardLinux.cpp
 *
 * This module contains the Linux specific part of the
 * implementation of the SystemAbstractions::Clipboard class.
 *
 * Copyright (c) 2017 by Richard Walters
 */

#include <string>
#include <SystemAbstractions/Clipboard.hpp>

namespace SystemAbstractions {

    /**
     * This is the Linux-specific state for the Clipboard class.
     */
    struct Clipboard::Impl {
    };

    Clipboard::Clipboard()
        : impl_(new Impl())
    {
    }

    Clipboard::~Clipboard() {
    }

    void Clipboard::Copy(const std::string& s) {
        selectedClipboardOperatingSystemInterface->Copy(s);
    }

    bool Clipboard::HasString() {
        return selectedClipboardOperatingSystemInterface->HasString();
    }

    std::string Clipboard::PasteString() {
        return selectedClipboardOperatingSystemInterface->PasteString();
    }

}

namespace {

    ClipboardOperatingSystemInterface clipboardLinuxInterface;

    /**
     * This isn't the actual "clipboard", but just a dummy
     * buffer to give the applications and tests the appearance
     * of there being a clipboard.
     */
    std::string fakeClipboard;

    /**
     * This flag indicates whether or not there is something
     * in the fake clipboard.
     */
    bool fakeClipboardFull = false;

}

void ClipboardOperatingSystemInterface::Copy(const std::string& s) {
    fakeClipboard = s;
    fakeClipboardFull = true;
}

bool ClipboardOperatingSystemInterface::HasString() {
    return fakeClipboardFull;
}

std::string ClipboardOperatingSystemInterface::PasteString() {
    if (fakeClipboardFull) {
        return fakeClipboard;
    } else {
        return "";
    }
}

ClipboardOperatingSystemInterface* selectedClipboardOperatingSystemInterface = &clipboardLinuxInterface;
