# SystemAbstractions

This is library contains classes which are either common utilities or wrap common operating system services whose interfaces vary from one operating system to another.

## Usage

The `SystemAbstractions::Clipboard` class is an abstraction of the "clipboard" feature available in most operating systems.

The `SystemAbstractions::DiagnosticsContext` class is a utility meant to be used with `SystemAbstractions::DiagnosticsSender`.  It enhances diagnostic messages generated during the scope of the context object, to help identify the context.

The `SystemAbstractions::DiagnosticsSender` class is a utility used to format and publish diagnostic messages in a large integrated system.  Delegate functions may subscribe to the sender in order to receive copies of any published messages.

The `SystemAbstractions::DiagnosticsStreamReporter` function is a utility meant to be used with `SystemAbstractions::DiagnosticsSender`.  It generates a delegate function which, when subscribed to a sender, prints all diagnostic messages to a pair of standard C `FILE` streams.  The stream printed to depends on the severity/importance level of the message.  A common use of this function is to print to the predefined standard C streams `stdout` and `stderr`, representing the standard output and standard error streams, respectively.

The `SystemAbstractions::DirectoryMonitor` class is an abstraction of operating system facilities allowing a program to subscribe to notifications sent when files in a given directory are created, modified, or deleted.

The `SystemAbstractions::DynamicLibrary` class is an abstraction of operating system facilities used to load, access, and unload run-time loadable modules, also known as dynamic-link libraries.

The `SystemAbstractions::File` class is both an implementation of the `SystemAbstractions::IFile` interface as well as a collection of cross-platform utility functions for performing common operations with directories and files.

The `SystemAbstractions::IFile` class is an abstract interface to an object which can be used as a "file", whether it's an actual file in the traditional sense of being in a file system, or it's merely a "file-like" object, having common file behaviors but not actually existing in any real file system.

The `SystemAbstractions::IFileCollection` class is an abstract interface to a collection of files, whether they are in an actual file system (as in an actual file system directory) or are held in some other implementation of a file collection, such as the contents of an archive or compressed file collection (e.g. ZIP file).

The `SystemAbstractions::NetworkConnection` class is an abstraction of a connection-oriented "socket" or "socket-like" object representing a connection between the program and some remote "peer", whether it be another program running on the same machine, a program running on a different machine on the same network, a remote server, or a cloud-based service.

The `SystemAbstractions::NetworkEndpoint` class is an abstraction of a connection-oriented or datagram-oriented "socket" or "socket-like" object representing a service provided by the program that is accessible by other programs and machines on the same network or a remote network.

The `SystemAbstractions::StringFile` class is an implementation of the `SystemAbstractions::IFile` interface in terms of a string in memory.

The `SystemAbstractions::Subprocess` class is a cross-platform utility for starting a child process and forming a parent-child connection (typically implemented as a pipe or socket in shared memory) in order for the parent process to monitor the child process for when it exits normally or crashes.

The `SystemAbstractions::TargetInfo` module contains functions which obtain basic information about the program and the machine hosting it, such as the processor architecture of the host machine and whether or not the program was built for debugging.

The `SystemAbstractions::Time` class is an abstraction of the time measurement facilities of the operating system, including both the system time (also called "the wall clock") and the high-precision counter typically provided by the processor.

## Supported platforms / recommended toolchains

This is a portable C++11 library which depends only on the C++11 compiler and standard library, so it should be supported on almost any platform.  The following are recommended toolchains for popular platforms.

* Windows -- [Visual Studio](https://www.visualstudio.com/) (Microsoft Visual C++)
* Linux -- clang or gcc
* MacOS -- Xcode (clang)

## Building

This library is not intended to stand alone.  It is intended to be included in a larger solution which uses [CMake](https://cmake.org/) to generate the build system and build applications which will link with the library.

There are two distinct steps in the build process:

1. Generation of the build system, using CMake
2. Compiling, linking, etc., using CMake-compatible toolchain

### Prerequisites

* [CMake](https://cmake.org/) version 3.8 or newer
* C++11 toolchain compatible with CMake for your development platform (e.g. [Visual Studio](https://www.visualstudio.com/) on Windows)
* [StringExtensions](https://github.com/rhymu8354/StringExtensions.git) - a
  library containing C++ string-oriented libraries, many of which ought to be
  in the standard library, but aren't.

### Build system generation

Generate the build system using [CMake](https://cmake.org/) from the solution root.  For example:

```bash
mkdir build
cd build
cmake -G "Visual Studio 15 2017" -A "x64" ..
```

### Compiling, linking, et cetera

Either use [CMake](https://cmake.org/) or your toolchain's IDE to build.
For [CMake](https://cmake.org/):

```bash
cd build
cmake --build . --config Release
```
