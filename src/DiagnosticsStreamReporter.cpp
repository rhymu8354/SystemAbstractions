/**
 * @file DiagnosticsStreamReporter.cpp
 *
 * This module contains the implementation of the
 * SystemAbstractions::DiagnosticsStreamReporter class.
 *
 * © 2014-2018 by Richard Walters
 */

#include <SystemAbstractions/DiagnosticsStreamReporter.hpp>
#include <SystemAbstractions/Time.hpp>

namespace SystemAbstractions {

    DiagnosticsSender::DiagnosticMessageDelegate DiagnosticsStreamReporter(
        FILE* output,
        FILE* error
    ) {
        auto time = std::make_shared< Time >();
        double timeReference = time->GetTime();
        return [
            output,
            error,
            time,
            timeReference
        ](
            std::string senderName,
            size_t level,
            std::string message
        ) {
            FILE* destination;
            std::string prefix;
            if (level >= DiagnosticsSender::Levels::ERROR) {
                destination = error;
                prefix = "error: ";
            } else if (level >= DiagnosticsSender::Levels::WARNING) {
                destination = error;
                prefix = "warning: ";
            } else {
                destination = output;
            }
            fprintf(
                destination,
                "[%.6lf %s:%zu] %s%s\n",
                time->GetTime() - timeReference,
                senderName.c_str(),
                level,
                prefix.c_str(),
                message.c_str()
            );
        };
    }

}
