#ifndef SYSTEM_ABSTRACTIONS_SUBPROCESS_HPP
#define SYSTEM_ABSTRACTIONS_SUBPROCESS_HPP

/**
 * @file Subprocess.hpp
 *
 * This module declares the SystemAbstractions::Subprocess class.
 *
 * Copyright (c) 2016 by Richard Walters
 */

#include <memory>
#include <string>
#include <vector>

namespace SystemAbstractions {

    /**
     * This class represents a child process started by the current process
     * and includes means of communicating between the two processes.
     */
    class Subprocess {
        // Custom Types
    public:
        /**
         * This is implemented by the owner of the object and
         * used to deliver callbacks.
         */
        class Owner {
        public:
            /**
             * @todo Needs documentation
             */
            virtual void SubprocessChildExited() {}

            /**
             * @todo Needs documentation
             */
            virtual void SubprocessChildCrashed() {}
        };

        // Public methods
    public:
        /**
         * This is an instance constructor.
         */
        Subprocess();

        /**
         * This is an instance constructor.
         *
         * @param impl
         *     This is an existing set of properties to encapsulate
         *     into a new object.
         */
        Subprocess(std::unique_ptr< struct SubprocessImpl >&& impl);

        /**
         * This is the instance move constructor.
         */
        Subprocess(Subprocess&& other);

        /**
         * This is the instance destructor.
         */
        ~Subprocess();

        /**
         * This is the move assignment operator.
         */
        Subprocess& operator=(Subprocess&& other);

        /**
         * @todo Needs documentation
         */
        bool StartChild(
            const std::string& program,
            const std::vector< std::string >& args,
            Owner* owner
        );

        /**
         * @todo Needs documentation
         */
        bool ContactParent(
            std::vector< std::string >& args,
            Owner* owner
        );

        // Disable copy constructor and assignment operator.
        Subprocess(const Subprocess&) = delete;
        Subprocess& operator=(const Subprocess&) = delete;

        // Private properties
    private:
        std::unique_ptr< struct SubprocessImpl > _impl;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_SUBPROCESS_HPP */
