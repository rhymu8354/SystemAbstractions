#ifndef SYSTEM_ABSTRACTIONS_TIME_HPP
#define SYSTEM_ABSTRACTIONS_TIME_HPP

/**
 * @file Time.hpp
 *
 * This module declares the SystemAbstractions::Time class.
 *
 * © 2016-2018 by Richard Walters
 */

#include <memory>
#include <time.h>

namespace SystemAbstractions {

    /**
     * This class contains methods dealing with time.
     */
    class Time {
        // Lifecycle management
    public:
        ~Time() noexcept;
        Time(const Time&) = delete;
        Time(Time&&) noexcept;
        Time& operator=(const Time&) = delete;
        Time& operator=(Time&&) noexcept;

        // Public methods
    public:
        /**
         * This is the instance constructor.
         */
        Time();

        /**
         * This method returns the amount of time, in seconds, that
         * has elapsed since this object was created.  The time is
         * measured using the system's high-precision perforance counter,
         * typically implemented in hardware (CPU).
         *
         * @return
         *     The amount of time, in seconds, that
         *     has elapsed since this object was created is returned.
         */
        double GetTime();

        /**
         * This method is an abstraction of the "localtime" function
         * whose name can vary from one operating system to another.
         * It returns the given or current time, in what the system
         * considers to be the "local time zone", using the C standard library
         * "struct tm" format.
         *
         * @param[in] time
         *     This is the number seconds since "the Epoch", as defined
         *     by POSIX (midnight UTC, January 1, 1970), for which to
         *     compute the "struct tm" format equivalent.  If zero,
         *     the current time is sampled and converted instead.
         *
         * @return
         *     The given or current time, in what the system considers to be
         *     the "local time zone", is returned, using the C standard
         *     library "struct tm" format.
         */
        static struct tm localtime(time_t time = 0);

        /**
         * This method is an abstraction of the "localtime" function
         * whose name can vary from one operating system to another.
         * It returns the given or current time, according to Coordinated
         * Universal Time (UTC), using the C standard library
         * "struct tm" format.
         *
         * @param[in] time
         *     This is the number seconds since "the Epoch", as defined
         *     by POSIX (midnight UTC, January 1, 1970), for which to
         *     compute the "struct tm" format equivalent.  If zero,
         *     the current time is sampled and converted instead.
         *
         * @return
         *     The given or current time, according to Coordinated
         *     Universal Time (UTC), is returned, using the C standard
         *     library "struct tm" format.
         */
        static struct tm gmtime(time_t time = 0);

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

#endif /* SYSTEM_ABSTRACTIONS_TIME_HPP */
