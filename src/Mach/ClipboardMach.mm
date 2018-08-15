/**
 * @file ClipboardMach.cpp
 *
 * This module contains the Mach implementation of the
 * SystemAbstractions::Clipboard class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#include <SystemAbstractions/Clipboard.hpp>

namespace SystemAbstractions {

    /**
     * This is the Mach-specific state for the Clipboard class.
     */
    struct Clipboard::Impl {
    };

    Clipboard::Clipboard()
        : impl_(new Impl())
    {
    }

    Clipboard::~Clipboard() noexcept = default;

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

    ClipboardOperatingSystemInterface clipboardMachInterface;

}

void ClipboardOperatingSystemInterface::Copy(const std::string& s) {
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    [pasteboard clearContents];
    [pasteboard writeObjects: @[[NSString stringWithUTF8String:s.c_str()]]];
}

bool ClipboardOperatingSystemInterface::HasString() {
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    return [pasteboard canReadObjectForClasses: @[[NSString class]] options: [NSDictionary dictionary]];
}

std::string ClipboardOperatingSystemInterface::PasteString() {
    NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
    NSArray* items = [pasteboard readObjectsForClasses: @[[NSString class]] options: [NSDictionary dictionary]];
    if (
        (items == nil)
        || (items.count <= 0)
        ) {
        return "";
    }
    return [items[0] UTF8String];
}

ClipboardOperatingSystemInterface* selectedClipboardOperatingSystemInterface = &clipboardMachInterface;

