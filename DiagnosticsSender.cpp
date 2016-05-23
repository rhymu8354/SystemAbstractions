/**
 * @file DiagnosticsSender.cpp
 *
 * This module contains the implementation of the
 * SystemAbstractions::DiagnosticsSender class.
 *
 * Copyright (c) 2014-2016 by Richard Walters
 */

#include "DiagnosticsSender.hpp"

#include <algorithm>
#include <limits>
#include <SystemAbstractions/StringExtensions.hpp>

namespace SystemAbstractions {

    DiagnosticsSender::DiagnosticsSender(std::string name)
        : _name(name)
        , _minLevel(std::numeric_limits< size_t >::max())
    {
    }

    void DiagnosticsSender::SubscribeToDiagnostics(DiagnosticsReceiver* subscriber, size_t minLevel) {
        std::lock_guard< std::mutex > lock(_mutex);
        _subscribers[subscriber] = { minLevel };
        _minLevel = std::min(_minLevel, minLevel);
    }

    void DiagnosticsSender::UnsubscribeFromDiagnostics(DiagnosticsReceiver* subscriber) {
        std::lock_guard< std::mutex > lock(_mutex);
        auto subscription = _subscribers.find(subscriber);
        if (subscription == _subscribers.end()) {
            return;
        }
        Subscription oldSubscription(subscription->second);
        (void)_subscribers.erase(subscription);
        if (oldSubscription.minLevel == _minLevel) {
            _minLevel = std::numeric_limits< size_t >::max();
            for (auto subscriber: _subscribers) {
                _minLevel = std::min(_minLevel, subscriber.second.minLevel);
            }
        }
    }

    size_t DiagnosticsSender::GetMinLevel() const {
        return _minLevel;
    }

    void DiagnosticsSender::SendDiagnosticInformationString(size_t level, std::string message) const {
        if (level < _minLevel) {
            return;
        }
        std::lock_guard< std::mutex > lock(_mutex);
        if (!_contextStack.empty()) {
            std::string contextChain;
            for (auto context: _contextStack) {
                contextChain = contextChain + context + ": ";
            }
            message = contextChain + message;
        }
        for (auto subscriber: _subscribers) {
            if (level >= subscriber.second.minLevel) {
                subscriber.first->ReceiveDiagnosticInformation(_name, level, message);
            }
        }
    }

    void DiagnosticsSender::SendDiagnosticInformationFormatted(size_t level, const char* format, ...) const {
        if (level < _minLevel) {
            return;
        }
        va_list args;
        va_start(args, format);
        const std::string message = SystemAbstractions::vsprintf(format, args);
        va_end(args);
        SendDiagnosticInformationString(level, message);
    }

    void DiagnosticsSender::PushContext(std::string context) {
        std::lock_guard< std::mutex > lock(_mutex);
        _contextStack.push_back(context);
    }

    void DiagnosticsSender::PopContext() {
        std::lock_guard< std::mutex > lock(_mutex);
        _contextStack.pop_back();
    }

    void DiagnosticsSender::ReceiveDiagnosticInformation(
        std::string senderName,
        size_t level,
        std::string message
    ) {
        if (level < _minLevel) {
            return;
        }
        std::lock_guard< std::mutex > lock(_mutex);
        if (!_contextStack.empty()) {
            std::string contextChain;
            for (auto context: _contextStack) {
                contextChain = contextChain + context + ": ";
            }
            message = contextChain + message;
        }
        for (auto subscriber: _subscribers) {
            if (level >= subscriber.second.minLevel) {
                subscriber.first->ReceiveDiagnosticInformation(_name + "/" + senderName, level, message);
            }
        }
    }

}
