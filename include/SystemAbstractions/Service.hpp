#ifndef SYSTEM_ABSTRACTIONS_SERVICE_HPP
#define SYSTEM_ABSTRACTIONS_SERVICE_HPP

/**
 * @file Service.hpp
 *
 * This module declares the SystemAbstractions::Service class.
 *
 * © 2018 by Richard Walters
 */

#include <memory>
#include <string>

namespace SystemAbstractions {

    /**
     * This is the base class for an operating system service or daemon
     * process, designed to be subclassed to make specific service
     * implementations.
     */
    class Service {
        // Lifecycle Management
    public:
        ~Service() noexcept;
        Service(const Service&) = delete;
        Service(Service&&) noexcept;
        Service& operator=(const Service&) = delete;
        Service& operator=(Service&&) noexcept;

        // Public methods
    public:
        /**
         * This is the instance constructor.
         */
        Service();

        /**
         * This method should be called in the context of the main thread
         * and/or function of the service, to hook into the operating system.
         * It does not return until the service should be terminated.
         *
         * @return
         *     The exit code that should be returned from the main function
         *     of the program is returned.
         */
        int Start();

        // Protected methods
    protected:
        /**
         * This method is called within in the context of the main thread
         * and/or function of the service, to perform the work of the service.
         * The method should not return until the service is to be stopped.
         *
         * @note
         *     If the service starts any worker threads, those worker threads
         *     must all be joined before this method returns.
         *
         * @return
         *     The exit code that should be returned from the main function
         *     of the program is returned.
         */
        virtual int Run() = 0;

        /**
         * This method is called to tell the service to stop.  The service
         * should promptly return from the Run method.
         */
        virtual void Stop() = 0;

        /**
         * This method returns the name to use for the service when a name
         * is required by the operating system.
         *
         * @return
         *     The name of the service is returned.
         */
        virtual std::string GetServiceName() const = 0;

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
        std::unique_ptr< Impl > impl_;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_SERVICE_HPP */
