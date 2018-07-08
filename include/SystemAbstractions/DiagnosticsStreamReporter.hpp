#ifndef SYSTEM_ABSTRACTIONS_DIAGNOSTICS_STREAM_REPORTER_HPP
#define SYSTEM_ABSTRACTIONS_DIAGNOSTICS_STREAM_REPORTER_HPP

/**
 * @file DiagnosticsStreamReporter.hpp
 *
 * This module declares the SystemAbstractions::DiagnosticsStreamReporter class.
 *
 * Copyright (c) 2014-2016 by Richard Walters
 */

#include "DiagnosticsReceiver.hpp"
#include "Time.hpp"

#include <stddef.h>
#include <stdio.h>

namespace SystemAbstractions {

    /**
     * This is a diagnostics receiver that prints diagnostic messages
     * to streams.
     */
    class DiagnosticsStreamReporter
        : public DiagnosticsReceiver
    {
        // Public methods
    public:
        DiagnosticsStreamReporter(FILE* output, FILE* error);

        // DiagnosticsReceiver
    public:
        void ReceiveDiagnosticInformation(
            std::string senderName,
            size_t level,
            std::string message
        );

        // Private properties
    private:
        FILE* _output;
        FILE* _error;
        Time _time;
        double _timeReference;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_DIAGNOSTICS_STREAM_REPORTER_HPP */
