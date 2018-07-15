/**
 * @file TimeWin32.cpp
 *
 * This module contains the Win32 specific part of the 
 * implementation of the SystemAbstractions::Time class.
 *
 * © 2016-2018 by Richard Walters
 */

/**
 * Windows.h should always be included first because other Windows header
 * files, such as KnownFolders.h, don't always define things properly if
 * you don't include Windows.h first.
 */
#include <Windows.h>

#include <SystemAbstractions/Time.hpp>

namespace SystemAbstractions {

    /**
     * This is the Win32-specific state for the Time class.
     */
    struct Time::Impl {
        double scale = 0.0;
    };

    Time::~Time() = default;
    Time::Time(Time&&) noexcept = default;
    Time& Time::operator=(Time&&) noexcept = default;

    Time::Time()
        : impl_(new Impl())
    {
    }

    double Time::GetTime() {
        if (impl_->scale == 0.0) {
            LARGE_INTEGER freq;
            (void)QueryPerformanceFrequency(&freq);
            impl_->scale = 1.0 / (double)freq.QuadPart;
        }
        LARGE_INTEGER now;
        (void)QueryPerformanceCounter(&now);
        return (double)now.QuadPart * impl_->scale;
    }

    struct tm Time::localtime(time_t time) {
        if (time == 0) {
            (void)::time(&time);
        }
        struct tm timeStruct;
        (void)localtime_s(&timeStruct, &time);
        return timeStruct;
    }

    struct tm Time::gmtime(time_t time) {
        if (time == 0) {
            (void)::time(&time);
        }
        struct tm timeStruct;
        (void)gmtime_s(&timeStruct, &time);
        return timeStruct;
    }

}
