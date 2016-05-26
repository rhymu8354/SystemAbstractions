/**
 * @file TimeLinux.cpp
 *
 * This module contains the Linux specific part of the 
 * implementation of the SystemAbstractions::Time class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "../Time.hpp"

#include <time.h>

namespace SystemAbstractions {

    /**
     * This is the Linux-specific state for the Time class.
     */
    struct TimeImpl {
    };

    Time::Time()
        : _impl(new TimeImpl())
    {
    }

    Time::~Time() {
    }

    double Time::GetTime() {
        struct timespec tp;
        if (clock_gettime(CLOCK_REALTIME, &tp) != 0) {
            return 0.0;
        }
        return (double)tp.tv_sec + (double)tp.tv_nsec / 1e9;
    }

}
