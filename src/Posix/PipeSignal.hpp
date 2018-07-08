#ifndef FILES_PIPE_SIGNAL_HPP
#define FILES_PIPE_SIGNAL_HPP

/**
 * @file PipeSignal.hpp
 *
 * This module declares the SystemAbstractions::PipeSignal class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include <memory>
#include <string>

namespace SystemAbstractions {

    /**
     * This class implements a level-sensitive signal that exposes
     * a file handle which may be used in select() to wait for
     * the signal.
     */
    class PipeSignal {
        // Public Methods
    public:
        /**
         * This is the instance constructor.
         */
        PipeSignal();

        /**
         * This is the instance destructor.
         */
        ~PipeSignal();

        /**
         * This method initializes the instance and must be called
         * before other methods besides the constructor.
         *
         * @return
         *     A flag indicating whether or not the method succeeded
         *     is returned.
         */
        bool Initialize();

        /**
         * This method returns a human-readable string indicating
         * the last error that occurred in another method of the instance.
         *
         * @return
         *     A human-readable string indicating the last error that
         *     occurred in another method of the instance is returned.
         */
        std::string GetLastError() const;

        /**
         * This method sets the signal.
         */
        void Set();

        /**
         * This method clears the signal.
         */
        void Clear();

        /**
         * This method returns an indication of whether or not the
         * signal is set.
         */
        bool IsSet() const;

        /**
         * This method returns a file handle which may be used with
         * the read set in select() to wait for the signal.
         *
         * @return
         *     A file handle which may be used with the read set in
         *     select() to wait for the signal is returned.
         */
        int GetSelectHandle() const;

        // Private Properties
    private:
        /**
         * This contains the private properties of the instance.
         */
         std::unique_ptr< struct PipeSignalImpl > _impl;
    };

}

#endif /* FILES_PIPE_SIGNAL_HPP */
