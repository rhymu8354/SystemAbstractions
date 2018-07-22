/**
 * @file ClipboardLinux.cpp
 *
 * This module contains the Linux specific part of the
 * implementation of the SystemAbstractions::Clipboard class.
 *
 * Copyright (c) 2017 by Richard Walters
 */

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
    }

    bool Clipboard::HasString() {
        return false;
    }

    std::string Clipboard::PasteString() {
        return "";
    }

}

namespace {

    ClipboardOperatingSystemInterface clipboardLinuxInterface;

}

ClipboardOperatingSystemInterface* selectedClipboardOperatingSystemInterface = &clipboardLinuxInterface;
