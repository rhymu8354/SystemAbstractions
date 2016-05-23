#ifndef SYSTEM_ABSTRACTIONS_DIAGNOSTICS_RECEIVER_HPP
#define SYSTEM_ABSTRACTIONS_DIAGNOSTICS_RECEIVER_HPP

/**
 * @file DiagnosticsReceiver.hpp
 *
 * This module declares the SystemAbstractions::DiagnosticsReceiver class.
 *
 * Copyright (c) 2014-2016 by Richard Walters
 */

#include <stddef.h>
#include <string>

namespace SystemAbstractions {

    /**
     * This represents an object that receives diagnostic information
     * from other objects.
     */
    class DiagnosticsReceiver {
        // Custom types
    public:
        enum Levels : size_t {
            WARNING = 5,
            ERROR = 10
        };

        // Public methods
    public:
        virtual void ReceiveDiagnosticInformation(
            std::string senderName,
            size_t level,
            std::string message
        ) = 0;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_DIAGNOSTICS_RECEIVER_HPP */
