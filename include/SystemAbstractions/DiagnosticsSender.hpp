#ifndef SYSTEM_ABSTRACTIONS_DIAGNOSTICS_SENDER_HPP
#define SYSTEM_ABSTRACTIONS_DIAGNOSTICS_SENDER_HPP

/**
 * @file DiagnosticsSender.hpp
 *
 * This module declares the SystemAbstractions::DiagnosticsSender class.
 *
 * © 2014-2018 by Richard Walters
 */

#include <functional>
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
    class DiagnosticsSender {
        // Types
    public:
        /**
         * These are informal level settings for common types
         * of messages such as warnings and errors.
         */
        enum Levels : size_t {
            WARNING = 5,
            ERROR = 10
        };

        /**
         * This is the type used for tokens which represent
         * subscriptions to diagnostic messages.
         */
        typedef unsigned int SubscriptionToken;

        /**
         * This is the type of function given when subscribing
         * to diagnostic messages, and called to deliver
         * any diagnostic messages published while the subscription
         * lasts.
         *
         * @param[in] senderName
         *     This identifies the origin of the diagnostic information.
         *
         * @param[in] level
         *     This is used to filter out less-important information.
         *     The level is higher the more important the information is.
         *
         * @param[in] message
         *     This is the content of the message.
         */
        typedef std::function<
            void(
                std::string senderName,
                size_t level,
                std::string message
            )
        > DiagnosticMessageDelegate;

        // Lifecycle Management
    public:
        ~DiagnosticsSender();
        DiagnosticsSender(const DiagnosticsSender&) = delete;
        DiagnosticsSender(DiagnosticsSender&&);
        DiagnosticsSender& operator=(const DiagnosticsSender&) = delete;
        DiagnosticsSender& operator=(DiagnosticsSender&&);

        // Public methods
    public:
        /**
         * This is the constructor.
         *
         * @param[in] name
         *     This is the name to provide as the source name for
         *     all diagnostic messages published by this object.
         */
        explicit DiagnosticsSender(std::string name);

        /**
         * This method forms a new subscription to diagnostic
         * messages published by the sender.
         *
         * @param[in] delegate
         *     This is the function to call to deliver messages
         *     to this subscriber.
         *
         * @param[in] minLevel
         *     This is the minimum level of message that this subscriber
         *     desires to receive.
         *
         * @return
         *     A token representing the subscription is returned.
         *     This may be passed to UnsubscribeFromDiagnostics
         *     in order to terminate the subscription.
         */
        SubscriptionToken SubscribeToDiagnostics(
            DiagnosticMessageDelegate delegate,
            size_t minLevel = 0
        );

        /**
         * This method terminates a subscription previously formed
         * by calling the SubscribeToDiagnostics method.
         *
         * @param[in] subscriptionToken
         *     This is the token returned from SubscribeToDiagnostics
         *     when the subscription was formed.
         */
        void UnsubscribeFromDiagnostics(SubscriptionToken subscriptionToken);

        /**
         * This method returns the lowest of all the minimum desired
         * message levels for all current subscribers.
         *
         * @return
         *     The lowest of all the minimum desired message levels
         *     for all current subscribers is returned.
         */
        size_t GetMinLevel() const;

        /**
         * This method publishes a static diagnostic message.
         *
         * @param[in] level
         *     This is used to filter out less-important information.
         *     The level is higher the more important the information is.
         *
         * @param[in] message
         *     This is the content of the message.
         */
        void SendDiagnosticInformationString(size_t level, std::string message) const;

        /**
         * This method publishes a diagnostic message formatted
         * according to the rules and capabilities of the C standard library
         * function "sprintf".
         *
         * @param[in] level
         *     This is used to filter out less-important information.
         *     The level is higher the more important the information is.
         *
         * @param[in] format
         *     This is the formatting string to use as a guide to build
         *     the message.
         */
        void SendDiagnosticInformationFormatted(size_t level, const char* format, ...) const;

        /**
         * This method adds the given string onto the top of the contextual
         * information stack for the sender.
         *
         * @param[in] context
         *     This is the string to push onto the contextural
         *     information stack.
         */
        void PushContext(std::string context);

        /**
         * This method removes the top string off of the contextual
         * information stack.
         */
        void PopContext();

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

#endif /* SYSTEM_ABSTRACTIONS_DIAGNOSTICS_SENDER_HPP */
