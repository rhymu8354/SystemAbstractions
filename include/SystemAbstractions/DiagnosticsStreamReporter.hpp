#ifndef SYSTEM_ABSTRACTIONS_DIAGNOSTICS_STREAM_REPORTER_HPP
#define SYSTEM_ABSTRACTIONS_DIAGNOSTICS_STREAM_REPORTER_HPP

/**
 * @file DiagnosticsStreamReporter.hpp
 *
 * This module declares the SystemAbstractions::DiagnosticsStreamReporter class.
 *
 * © 2014-2018 by Richard Walters
 */

#include "DiagnosticsSender.hpp"

#include <stdio.h>

namespace SystemAbstractions {

    /**
     * This function returns a new diagnostic message delegate which
     * formats and prints all received diagnostic messages to the given
     * log files, according to the time received, the level indicated,
     * and the received message text.
     *
     * @param[in] output
     *     This is the file to which to print all diagnostic messages
     *     with levels that are under the "Levels::WARNING" level informally
     *     defined in the DiagnosticsSender class.
     *
     * @param[in] error
     *     This is the file to which to print all diagnostic messages
     *     with levels that are at or over the "Levels::WARNING" level
     *     informally defined in the DiagnosticsSender class.
     */
    DiagnosticsSender::DiagnosticMessageDelegate DiagnosticsStreamReporter(
        FILE* output,
        FILE* error
    );

}

#endif /* SYSTEM_ABSTRACTIONS_DIAGNOSTICS_STREAM_REPORTER_HPP */
