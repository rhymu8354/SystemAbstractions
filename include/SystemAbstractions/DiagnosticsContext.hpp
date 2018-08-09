#ifndef SYSTEM_ABSTRACTIONS_DIAGNOSTICS_CONTEXT_HPP
#define SYSTEM_ABSTRACTIONS_DIAGNOSTICS_CONTEXT_HPP

/**
 * @file DiagnosticsContext.hpp
 *
 * This module declares the SystemAbstractions::DiagnosticsContext class.
 *
 * © 2014-2018 by Richard Walters
 */

#include "DiagnosticsSender.hpp"

#include <string>

namespace SystemAbstractions {

    /**
     * This is a helper object which pushes a string onto the context
     * stack of a diagnostic sender.  The string pushed is popped when
     * the helper object is destroyed.
     */
    class DiagnosticsContext {
        // Lifecycle Management
    public:
        ~DiagnosticsContext() noexcept;
        DiagnosticsContext(const DiagnosticsContext&) = delete;
        DiagnosticsContext(DiagnosticsContext&&) noexcept = delete;
        DiagnosticsContext& operator=(const DiagnosticsContext&) = delete;
        DiagnosticsContext& operator=(DiagnosticsContext&&) noexcept = delete;

        // Public methods
    public:
        /**
         * This is the constructor.
         *
         * @param[in] diagnosticsSender
         *     This is the sender upon which this class is pushing a context
         *     as long as the class instance exists.
         *
         * @param[in] context
         *     This is the string to push onto the context stack for the
         *     given diagnostics sender.  It will be popped off the same stack
         *     when this object is destroyed.
         */
        DiagnosticsContext(
            DiagnosticsSender& diagnosticsSender,
            const std::string& context
        ) noexcept;

        // Private properties
    private:
        /**
         * This is the type of structure that contains the private
         * properties of the instance.  It is defined in the implementation
         * and declared here to ensure that it is scoped inside the class.
         */
        struct Impl;

        /**
         * This contains the private properties of the instance.
         */
        std::unique_ptr< struct Impl > impl_;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_DIAGNOSTICS_CONTEXT_HPP */
