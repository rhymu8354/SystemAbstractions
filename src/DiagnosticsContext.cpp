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

    /**
     * This holds the private properties of the DiagnosticsContext class.
     */
    struct DiagnosticsContext::Impl {
        // Properties

        /**
         * This is the sender upon which this class is pushing a context
         * as long as the class instance exists.
         */
        DiagnosticsSender& diagnosticsSender;

        // Methods

        /**
         * This is the constructor.
         *
         * @param[in] DiagnosticsSender& newDiagnosticsSender
         *     This is the sender upon which this class is pushing a context
         *     as long as the class instance exists.
         */
        Impl(DiagnosticsSender& newDiagnosticsSender)
            : diagnosticsSender(newDiagnosticsSender)
        {
        }
    };

    DiagnosticsContext::DiagnosticsContext(
        DiagnosticsSender& diagnosticsSender,
        const std::string& context
    ) noexcept
        : impl_(new Impl(diagnosticsSender))
    {
        diagnosticsSender.PushContext(context);
    }

    DiagnosticsContext::~DiagnosticsContext() noexcept {
        impl_->diagnosticsSender.PopContext();
    }

}
