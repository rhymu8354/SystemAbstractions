#ifndef SYSTEM_ABSTRACTIONS_SUBPROCESS_HPP
#define SYSTEM_ABSTRACTIONS_SUBPROCESS_HPP

/**
 * @file Subprocess.hpp
 *
 * This module declares the SystemAbstractions::Subprocess class.
 *
 * © 2016-2018 by Richard Walters
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
             * This is callback that is called if the child process
             * exits normally (without crashing).
             */
            virtual void SubprocessChildExited() {}

            /**
             * This is callback that is called if the child process
             * exits abnormally (crashes).
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
        Subprocess(std::unique_ptr< struct SubprocessImpl >&& impl) noexcept;

        /**
         * This is the instance move constructor.
         */
        Subprocess(Subprocess&& other) noexcept;

        /**
         * This is the instance destructor.
         */
        ~Subprocess();

        /**
         * This is the move assignment operator.
         */
        Subprocess& operator=(Subprocess&& other) noexcept;

        /**
         * This method starts the subprocess and establishes
         * a line of communication (a pipe) with it in order to
         * detect when the child exits or crashes.
         *
         * @note
         *     The actual command line given to the subprocess
         *     will have two additional arguments at the front:
         *     - "child" -- to let the subprocess know that it is
         *       running as a child (subprocess).
         *     - (pipe handle) -- identifier of a global pipe
         *       the subprocess can open and use to communicate
         *       with the parent process.
         *
         * @param[in] program
         *     This is the path and name of the program to
         *     run in the subprocess.
         *
         * @param[in] args
         *     These are the command-line arguments to pass to
         *     the subprocess.
         *
         * @param[in] owner
         *     This object is used to issue callbacks when
         *     the subprocess exits or crashes.
         *
         * @return
         *     An indication of whether or not the child process
         *     was started successfully is returned.
         */
        bool StartChild(
            std::string program,
            const std::vector< std::string >& args,
            Owner* owner
        );

        /**
         * This is the function called by the subprocess
         * in order to establish a link of communication back
         * to the parent process, so that the parent process
         * can know when the child process exits or crashes.
         *
         * @param[in,out] args
         *     On input, this is the original command-line arguments
         *     to the child process, minus the name of the program,
         *     which is typically the first command-line argument.
         *
         *     On output, this is the remainder of the command-line
         *     arguments, after the two special arguments at the
         *     front are removed (see note in StartChild()).
         *
         * @return
         *     An indication of whether or not the child process
         *     was able to contact the parent successfully is returned.
         */
        bool ContactParent(std::vector< std::string >& args);

        // Disable copy constructor and assignment operator.
        Subprocess(const Subprocess&) = delete;
        Subprocess& operator=(const Subprocess&) = delete;

        // Private properties
    private:
        std::unique_ptr< struct SubprocessImpl > _impl;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_SUBPROCESS_HPP */
