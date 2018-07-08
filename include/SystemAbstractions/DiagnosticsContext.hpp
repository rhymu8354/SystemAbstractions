#ifndef SYSTEM_ABSTRACTIONS_DIAGNOSTICS_CONTEXT_HPP
#define SYSTEM_ABSTRACTIONS_DIAGNOSTICS_CONTEXT_HPP

/**
 * @file DiagnosticsContext.hpp
 *
 * This module declares the SystemAbstractions::DiagnosticsContext class.
 *
 * Copyright (c) 2014-2016 by Richard Walters
 */

#include "DiagnosticsSender.hpp"

#include <map>
#include <stdarg.h>
#include <string>

namespace SystemAbstractions {

    /**
     * This is a helper object which pushes a string onto the context
     * stack of a diagnostic sender.  The string pushed is popped when
     * the helper object is destroyed.
     */
    class DiagnosticsContext {
        // Public methods
    public:
        explicit DiagnosticsContext(DiagnosticsSender& diagnosticsSender, std::string context);
        ~DiagnosticsContext();

        // Private properties
    private:
        DiagnosticsSender& _diagnosticsSender;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_DIAGNOSTICS_CONTEXT_HPP */
