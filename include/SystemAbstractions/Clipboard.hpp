#ifndef SYSTEM_ABSTRACTIONS_CLIPBOARD_HPP
#define SYSTEM_ABSTRACTIONS_CLIPBOARD_HPP

/**
 * @file Clipboard.hpp
 *
 * This module declares the SystemAbstractions::Clipboard class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include <memory>
#include <string>

namespace SystemAbstractions {

    /**
     * This class represents a file accessed through the
     * native operating system.
     */
    class Clipboard {
        // Public methods
    public:
        /**
         * This is the default constructor.
         */
        Clipboard();

        /**
         * This is the destructor.
         */
        ~Clipboard();

        /**
         * This is the copy constructor.
         *
         * @param[in] other
         *     This is the other object to copy.
         */
        Clipboard(const Clipboard& other) = default;

        /**
         * This is the move constructor.
         *
         * @param[in] other
         *     This is the other object to move.
         */
        Clipboard(Clipboard&& other) = default;

        /**
         * This is the copy-assignment operator.
         *
         * @param[in] other
         *     This is the other object to copy.
         */
        Clipboard& operator=(const Clipboard& other) = default;

        /**
         * This is the move-assignment operator.
         *
         * @param[in] other
         *     This is the other object to move.
         */
        Clipboard& operator=(Clipboard&& other) = default;

        /**
         * This puts the given string into the clipboard.
         *
         * @param[in] s
         *     This is the string to put into the clipboard.
         */
        void Copy(const std::string& s);

        /**
         * This method determines whether or not the clipboard's contents
         * can be represented by a string.
         *
         * @return
         *     An indication of whether or not the clipboard's contents
         *     can be represented by a string is returned.
         */
        bool HasString();

        /**
         * This method returns the contents of the clipboard as a string.
         *
         * @return
         *     The contents of the clipboard as a string is returned.
         */
        std::string PasteString();

        // Private properties
    private:
        /**
         * This contains any platform-specific state for the object.
         */
        struct Impl;

        /**
         * This contains any platform-specific state for the object.
         */
        std::unique_ptr< Impl > impl_;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_CLIPBOARD_HPP */
