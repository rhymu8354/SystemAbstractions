#ifndef SYSTEM_ABSTRACTIONS_DIAGNOSTICS_SENDER_HPP
#define SYSTEM_ABSTRACTIONS_DIAGNOSTICS_SENDER_HPP

/**
 * @file DiagnosticsSender.hpp
 *
 * This module declares the SystemAbstractions::DiagnosticsSender class.
 *
 * Copyright (c) 2014-2016 by Richard Walters
 */

#include "DiagnosticsReceiver.hpp"

#include <map>
#include <mutex>
#include <deque>
#include <stdarg.h>
#include <string>

namespace SystemAbstractions {

    /**
     * This represents an object that sends diagnostic information
     * to other objects.
     */
    class DiagnosticsSender
        : public DiagnosticsReceiver
    {
        // Custom types
    public:
        struct Subscription {
            size_t minLevel;
        };

        // Public methods
    public:
        explicit DiagnosticsSender(std::string name);
        void SubscribeToDiagnostics(DiagnosticsReceiver* subscriber, size_t minLevel = 0);
        void UnsubscribeFromDiagnostics(DiagnosticsReceiver* subscriber);
        size_t GetMinLevel() const;
        void SendDiagnosticInformationString(size_t level, std::string message) const;
        void SendDiagnosticInformationFormatted(size_t level, const char* format, ...) const;
        void PushContext(std::string context);
        void PopContext();

        // DiagnosticsReceiver
    public:
        virtual void ReceiveDiagnosticInformation(
            std::string senderName,
            size_t level,
            std::string message
        ) override;

        // Private properties
    private:
        std::string _name;
        std::map< DiagnosticsReceiver*, Subscription > _subscribers;
        size_t _minLevel;
        std::deque< std::string > _contextStack;
        mutable std::mutex _mutex;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_DIAGNOSTICS_SENDER_HPP */
