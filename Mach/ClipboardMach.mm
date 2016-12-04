/**
 * @file ClipboardMach.cpp
 *
 * This module contains the Mach implementation of the
 * SystemAbstractions::Clipboard class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "../Clipboard.hpp"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

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

    Clipboard::~Clipboard() {
    }

    void Clipboard::Copy(const std::string& s) {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard clearContents];
        [pasteboard writeObjects: @[[NSString stringWithUTF8String:s.c_str()]]];
    }

    bool Clipboard::HasString() {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        return [pasteboard canReadObjectForClasses: @[[NSString class]] options: [NSDictionary dictionary]];
    }

    std::string Clipboard::PasteString() {
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

}
