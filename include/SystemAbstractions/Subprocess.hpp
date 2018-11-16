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
#include <set>
#include <stdint.h>
#include <string>
#include <vector>

namespace SystemAbstractions {

    /**
     * This class represents a child process started by the current process
     * and includes means of communicating between the two processes.
     */
    class Subprocess {
        // Types
    public:
        /**
         * This holds information about an operating system process.
         */
        struct ProcessInfo {
            /**
             * This is the identifier of the process, assigned by the operating
             * system.
             */
            unsigned int id;

            /**
             * This is the path to the executable image the process is
             * executing.
             */
            std::string image;

            /**
             * These are the TCP port numbers on which the process
             * is listening for connections.
             */
            std::set< uint16_t > tcpServerPorts;
        };

        // Lifecycle Management
    public:
        ~Subprocess() noexcept;
        Subprocess(const Subprocess&) = delete;
        Subprocess(Subprocess&&) noexcept;
        Subprocess& operator=(const Subprocess&) = delete;
        Subprocess& operator=(Subprocess&&) noexcept;

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
         *     The process ID of the new subprocess is returned.
         *
         * @retval 0
         *     This is returned if the subprocess could not be started.
         */
        unsigned int StartChild(
            std::string program,
            const std::vector< std::string >& args,
            std::function< void() > childExited,
            std::function< void() > childCrashed
        );

        /**
         * This method starts a process completely detached from the
         * current process, as in there is no line of communication.
         *
         * @param[in] program
         *     This is the path and name of the program to
         *     run in the process.
         *
         * @param[in] args
         *     These are the command-line arguments to pass to
         *     the process.
         *
         * @return
         *     The process ID of the new process is returned.
         *
         * @retval 0
         *     This is returned if the process could not be started.
         */
        static unsigned int StartDetached(
            std::string program,
            const std::vector< std::string >& args
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

        /**
         * This function returns the identifier of the current process.
         *
         * @return
         *     The current process identifier is returned.
         */
        static unsigned int GetCurrentProcessId();

        /**
         * This function gathers information about all the processes currently
         * running in the system.
         *
         * @return
         *     A collection of structures containing information about each
         *     process currently running in the system is returned.
         */
        static std::vector< ProcessInfo > GetProcessList();

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
        std::unique_ptr< Impl > impl_;
    };

}

#endif /* SYSTEM_ABSTRACTIONS_SUBPROCESS_HPP */
