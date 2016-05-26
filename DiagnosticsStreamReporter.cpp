/**
 * @file DiagnosticsStreamReporter.cpp
 *
 * This module contains the implementation of the
 * SystemAbstractions::DiagnosticsStreamReporter class.
 *
 * Copyright (c) 2014-2016 by Richard Walters
 */

#include "DiagnosticsStreamReporter.hpp"

namespace SystemAbstractions {

    DiagnosticsStreamReporter::DiagnosticsStreamReporter(FILE* output, FILE* error)
        : _output(output)
        , _error(error)
    {
        _timeReference = _time.GetTime();
    }

    void DiagnosticsStreamReporter::ReceiveDiagnosticInformation(
        std::string senderName,
        size_t level,
        std::string message
    ) {
        FILE* destination;
        std::string prefix;
        if (level >= DiagnosticsReceiver::Levels::ERROR) {
            destination = _error;
            prefix = "error: ";
        } else if (level >= DiagnosticsReceiver::Levels::WARNING) {
            destination = _error;
            prefix = "warning: ";
        } else {
            destination = _output;
        }
        fprintf(destination, "[%.6lf %s:%u] %s%s\n", _time.GetTime() - _timeReference, senderName.c_str(), (unsigned int)level, prefix.c_str(),  message.c_str());
    }

}
