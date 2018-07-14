#ifndef SYSTEM_ABSTRACTIONS_SUBPROCESS_HPP
#define SYSTEM_ABSTRACTIONS_SUBPROCESS_HPP

/**
 * @file Subprocess.hpp
 *
 * This module declares the SystemAbstractions::Subprocess class.
 *
 * © 2016-2018 by Richard Walters
 */

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace SystemAbstractions {

    /**
     * This class represents a child process started by the current process
     * and includes means of communicating between the two processes.
     */
    class Subprocess {
        // Lifecycle Management
    public:
        ~Subprocess();
        Subprocess(const Subprocess&) = delete;
        Subprocess(Subprocess&&);
        Subprocess& operator=(const Subprocess&) = delete;
        Subprocess& operator=(Subprocess&&);

        // Public methods
    public:
        /**
         * This is an instance constructor.
         */
        Subprocess();

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
         * @param[in] childExited
         *     This is a callback to call if the child process
         *     exits normally (without crashing).
         *
         * @param[in] childCrashed
         *     This is a callback to call if the child process
         *     exits abnormally (crashes).
         *
         * @return
         *     An indication of whether or not the child process
         *     was started successfully is returned.
         */
        bool StartChild(
            std::string program,
            const std::vector< std::string >& args,
            std::function< void() > childExited,
            std::function< void() > childCrashed
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

#endif /* SYSTEM_ABSTRACTIONS_SUBPROCESS_HPP */
