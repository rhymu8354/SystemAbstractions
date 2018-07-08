/**
 * @file DiagnosticsContext.cpp
 *
 * This module contains the implementation of the
 * SystemAbstractions::DiagnosticsContext class.
 *
 * Copyright (c) 2014-2016 by Richard Walters
 */

#include <SystemAbstractions/DiagnosticsContext.hpp>

namespace SystemAbstractions {

    DiagnosticsContext::DiagnosticsContext(DiagnosticsSender& diagnosticsSender, std::string context)
        : _diagnosticsSender(diagnosticsSender)
    {
        _diagnosticsSender.PushContext(context);
    }

    DiagnosticsContext::~DiagnosticsContext() {
        _diagnosticsSender.PopContext();
    }

}
