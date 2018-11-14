/**
 * @file ServiceLinux.cpp
 *
 * This module contains the Linux implementation of the
 * SystemAbstractions::Service class.
 *
 * © 2018 by Richard Walters
 */

#include <future>
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
         * This is used to tell the worker thread to stop early (before the
         * "shutDown" flag is set.
         */
        std::promise< void > stopWorker;

        /**
         * This points back to the instance's interface.
         */
        Service* instance;

        /**
         * This runs PollShutDown as a worker thread.
         */
        std::thread worker;

        /**
         * This method runs as a worker thread, polling the "shutDown" flag
         * until it's set or the thread is told to stop.
         */
        void PollShutDown() {
            const auto workerToldToStop = stopWorker.get_future();
            while (!shutDown) {
                if (workerToldToStop.wait_for(std::chrono::milliseconds(100)) == std::future_status::ready) {
                    break;
                }
            }
            instance->Stop();
        }
    };

    Service::~Service() noexcept = default;
    Service::Service(Service&& other) noexcept
        : impl_(std::move(other.impl_))
    {
        impl_->instance = this;
    }
    Service& Service::operator=(Service&& other) noexcept {
        impl_ = std::move(other.impl_);
        impl_->instance = this;
        return *this;
    }

    Service::Service()
        : impl_(new Impl())
    {
        impl_->instance = this;
    }

    int Service::Start() {
        const auto previousInterruptHandler = signal(SIGTERM, InterruptHandler);
        impl_->worker = std::thread(&Impl::PollShutDown, impl_.get());
        const int result = Run();
        impl_->stopWorker.set_value();
        impl_->worker.join();
        (void)signal(SIGTERM, previousInterruptHandler);
        return result;
    }

}
