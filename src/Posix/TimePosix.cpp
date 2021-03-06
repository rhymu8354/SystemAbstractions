/**
 * @file TimePosix.cpp
 *
 * This module contains the POSIX specific part of the
 * implementation of the SystemAbstractions::Time class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include <SystemAbstractions/Time.hpp>

namespace SystemAbstractions {

    struct tm Time::localtime(time_t time) {
        if (time == 0) {
            (void)::time(&time);
        }
        struct tm timeStruct;
        (void)localtime_r(&time, &timeStruct);
        return timeStruct;
    }

    struct tm Time::gmtime(time_t time) {
        if (time == 0) {
            (void)::time(&time);
        }
        struct tm timeStruct;
        (void)gmtime_r(&time, &timeStruct);
        return timeStruct;
    }

}
