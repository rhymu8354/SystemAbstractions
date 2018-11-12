/**
 * @file ServicePosix.cpp
 *
 * This module contains the POSIX implementation of the
 * SystemAbstractions::Service class.
 *
 * © 2018 by Richard Walters
 */

#include <SystemAbstractions/Service.hpp>

namespace SystemAbstractions {

    /**
     * This structure contains the private methods and properties of
     * the Service class.
     */
    struct Service::Impl {
    };

    Service::~Service() noexcept = default;
    Service::Service(Service&&) noexcept = default;
    Service& Service::operator=(Service&&) noexcept = default;

    Service::Service()
        : impl_(new Impl())
    {
    }

    int Service::Start() {
        return 0;
    }

}