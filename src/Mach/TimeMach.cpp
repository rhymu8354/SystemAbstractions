/**
 * @file TimeMach.cpp
 *
 * This module contains the Mach specific part of the 
 * implementation of the SystemAbstractions::Time class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include "../Time.hpp"

#include <mach/mach.h>
#include <mach/mach_time.h>

namespace SystemAbstractions {

    /**
     * This is the Mach-specific state for the Time class.
     */
    struct TimeImpl {
        double scale = 0.0;
    };

    Time::Time()
        : _impl(new TimeImpl())
    {
    }

    Time::~Time() {
    }

    double Time::GetTime() {
        if (_impl->scale == 0.0) {
            mach_timebase_info_data_t timebaseInfo;
            (void)mach_timebase_info(&timebaseInfo);
            _impl->scale = timebaseInfo.numer / (1e9 * timebaseInfo.denom);
        }
        return (double)mach_absolute_time() * _impl->scale;
    }

}
