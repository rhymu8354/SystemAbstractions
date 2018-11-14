/**
 * @file ServiceLinux.cpp
 *
 * This module contains the Linux implementation of the
 * SystemAbstractions::Service class.
 *
 * © 2018 by Richard Walters
 */

#include <signal.h>
#include <SystemAbstractions/Service.hpp>
#include <thread>

namespace {

    /**
     * This flag indicates whether or not the service should shut down.
     */
    bool shutDown = false;

    /**
     * This function is set up to be called when the SIGTERM signal is
     * received by the program.  It just sets the "shutDown" flag
     * and relies on the service to be polling the flag to detect
     * when it's been set.
     *
     * @param[in] sig
     *     This is the signal for which this function was called.
     */
    void InterruptHandler(int) {
        shutDown = true;
    }

}

namespace SystemAbstractions {

    /**
     * This structure contains the private methods and properties of
     * the Service class.
     */
    struct Service::Impl {
        /**
         * This points back to the instance's interface.
         */
        Service* instance;

        /**
         * This runs PollShutdown as a worker thread.
         */
        std::thread worker;

        /**
         * This method runs as a worker thread, polling the "shutdown" flag
         * until it's set or the thread is told to stop.
         */
        void PollShutdown() {
            while (!shutDown) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            instance->Stop();
        }
    };

    Service::~Service() noexcept = default;
    Service::Service(Service&&) noexcept = default;
    Service& Service::operator=(Service&&) noexcept = default;

    Service::Service()
        : impl_(new Impl())
    {
        impl_->instance = this;
    }

    int Service::Start() {
        const auto previousInterruptHandler = signal(SIGTERM, InterruptHandler);
        impl_->worker = std::thread(&Impl::PollShutdown, impl_.get());
        const int result = Run();
        impl_->worker.join();
        (void)signal(SIGTERM, previousInterruptHandler);
        return result;
    }

}
