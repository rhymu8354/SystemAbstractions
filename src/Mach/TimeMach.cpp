/**
 * @file TimeMach.cpp
 *
 * This module contains the Mach specific part of the 
 * implementation of the SystemAbstractions::Time class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include <mach/mach.h>
#include <mach/mach_time.h>
#include <SystemAbstractions/Time.hpp>

namespace SystemAbstractions {

    /**
     * This is the Mach-specific state for the Time class.
     */
    struct Time::Impl {
        double scale = 0.0;
    };

    Time::Time()
        : impl_(new Impl())
    {
    }

    Time::~Time() noexcept = default;

    double Time::GetTime() {
        if (impl_->scale == 0.0) {
            mach_timebase_info_data_t timebaseInfo;
            (void)mach_timebase_info(&timebaseInfo);
            impl_->scale = timebaseInfo.numer / (1e9 * timebaseInfo.denom);
        }
        return (double)mach_absolute_time() * impl_->scale;
    }

}
