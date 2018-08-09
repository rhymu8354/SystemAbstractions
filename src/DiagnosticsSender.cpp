/**
 * @file DiagnosticsSender.cpp
 *
 * This module contains the implementation of the
 * SystemAbstractions::DiagnosticsSender class.
 *
 * Copyright (c) 2014-2016 by Richard Walters
 */

#include <SystemAbstractions/DiagnosticsSender.hpp>
#include <SystemAbstractions/StringExtensions.hpp>

#include <algorithm>
#include <limits>

namespace {

    /**
     * This is used internally by DiagnosticsSender to track
     * subscriptions.
     */
    typedef unsigned int SubscriptionToken;

    /**
     * This holds information about a single subscriber
     * to a DiagnosticsSender's messages.
     */
    struct Subscription {
        // Properties

        /**
         * This is the function to call to deliver messages
         * to this subscriber.
         */
        SystemAbstractions::DiagnosticsSender::DiagnosticMessageDelegate delegate;

        /**
         * This is the minimum level of message that this subscriber
         * desires to receive.
         */
        size_t minLevel = 0;

        // Methods

        /**
         * This constructor sets all properties to their defaults.
         */
        Subscription() = default;

        /**
         * This constructor is used to initialize all the properties
         * of the instance.
         *
         * @param[in] newDelegate
         *     This is the function to call to deliver messages
         *     to this subscriber.
         *
         * @param[in] newMinLevel
         *     This is the minimum level of message that this subscriber
         *     desires to receive.
         */
        Subscription(
            SystemAbstractions::DiagnosticsSender::DiagnosticMessageDelegate newDelegate,
            size_t newMinLevel
        )
            : delegate(newDelegate)
            , minLevel(newMinLevel)
        {
        }
    };

}

namespace SystemAbstractions {

    /**
     * This holds the private properties of the DiagnosticsSender class.
     */
    struct DiagnosticsSender::Impl {
        /**
         * This is the name of the sender, provided in all
         * messages published from the sender.
         */
        std::string name;

        /**
         * These are the current valid subscriptions
         * to the diagnostic messages published by this sender.
         */
        std::map< SubscriptionToken, Subscription > subscribers;

        /**
         * This is the next token to use for the next
         * subscription formed for this sender.
         */
        SubscriptionToken nextSubscriptionToken = 1;

        /**
         * This is the minimum of all minimum desired message
         * levels for all current subscribers.
         */
        size_t minLevel = std::numeric_limits< size_t >::max();

        /**
         * This is used to include extra contextual information
         * in front of any messages published from this sender.
         */
        std::deque< std::string > contextStack;

        /**
         * This is used to synchronize access to this object.
         */
        mutable std::mutex mutex;
    };

    DiagnosticsSender::~DiagnosticsSender() noexcept = default;
    DiagnosticsSender::DiagnosticsSender(DiagnosticsSender&&) noexcept = default;
    DiagnosticsSender& DiagnosticsSender::operator=(DiagnosticsSender&&) noexcept = default;

    DiagnosticsSender::DiagnosticsSender(std::string name)
        : impl_(new Impl())
    {
        impl_->name = name;
    }

    auto DiagnosticsSender::SubscribeToDiagnostics(DiagnosticMessageDelegate delegate, size_t minLevel) -> UnsubscribeDelegate {
        std::lock_guard< std::mutex > lock(impl_->mutex);
        const auto subscriptionToken = impl_->nextSubscriptionToken++;
        impl_->subscribers[subscriptionToken] = { delegate, minLevel };
        impl_->minLevel = std::min(impl_->minLevel, minLevel);
        return [this, subscriptionToken]{
            std::lock_guard< std::mutex > lock(impl_->mutex);
            auto subscription = impl_->subscribers.find(subscriptionToken);
            if (subscription == impl_->subscribers.end()) {
                return;
            }
            Subscription oldSubscription(subscription->second);
            (void)impl_->subscribers.erase(subscription);
            if (oldSubscription.minLevel == impl_->minLevel) {
                impl_->minLevel = std::numeric_limits< size_t >::max();
                for (auto subscriber: impl_->subscribers) {
                    impl_->minLevel = std::min(impl_->minLevel, subscriber.second.minLevel);
                }
            }
        };
    }

    size_t DiagnosticsSender::GetMinLevel() const {
        return impl_->minLevel;
    }

    void DiagnosticsSender::SendDiagnosticInformationString(size_t level, std::string message) const {
        if (level < impl_->minLevel) {
            return;
        }
        std::lock_guard< std::mutex > lock(impl_->mutex);
        if (!impl_->contextStack.empty()) {
            std::string contextChain;
            for (auto context: impl_->contextStack) {
                contextChain = contextChain + context + ": ";
            }
            message = contextChain + message;
        }
        for (auto subscriber: impl_->subscribers) {
            if (level >= subscriber.second.minLevel) {
                subscriber.second.delegate(impl_->name, level, message);
            }
        }
    }

    void DiagnosticsSender::SendDiagnosticInformationFormatted(size_t level, const char* format, ...) const {
        if (level < impl_->minLevel) {
            return;
        }
        va_list args;
        va_start(args, format);
        const std::string message = SystemAbstractions::vsprintf(format, args);
        va_end(args);
        SendDiagnosticInformationString(level, message);
    }

    void DiagnosticsSender::PushContext(std::string context) {
        std::lock_guard< std::mutex > lock(impl_->mutex);
        impl_->contextStack.push_back(context);
    }

    void DiagnosticsSender::PopContext() {
        std::lock_guard< std::mutex > lock(impl_->mutex);
        impl_->contextStack.pop_back();
    }

}
