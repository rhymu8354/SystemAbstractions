#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <SystemAbstractions/File.hpp>
#include <SystemAbstractions/Subprocess.hpp>

#ifndef _WIN32
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#endif /* not _WIN32 */

#ifdef __APPLE__
#include <libproc.h>
#include <sys/proc_info.h>
#endif /* __APPLE__ */

int main(int argc, char* argv[]) {
    const std::string pidFilePath = SystemAbstractions::File::GetExeParentDirectory() + "/TestArea/pid";
    auto pidFile = fopen(pidFilePath.c_str(), "w");
    (void)fprintf(pidFile, "%u", SystemAbstractions::Subprocess::GetCurrentProcessId());
    (void)fclose(pidFile);
    SystemAbstractions::Subprocess parent;
    std::vector< std::string > args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }
#ifdef _WIN32
#elif defined(__APPLE__)
    int pipeToParentFd;
#else /* Linux */
    std::string pipeToParentFdPath;
#endif /* various platforms */
    if (
        (args.size() >= 1)
        && (args[0] == "child")
    ) {
#ifdef _WIN32
#elif defined(__APPLE__)
        if (sscanf(args[1].c_str(), "%d", &pipeToParentFd) != 1) {
            return EXIT_FAILURE;
        }
#else /* Linux */
        pipeToParentFdPath = "/proc/self/fd/" + args[1];
#endif /* various platforms */
        if (!parent.ContactParent(args)) {
            return EXIT_FAILURE;
        }
    } else if (
        (args.size() >= 1)
        && (args[0] == "detached")
    ) {
#ifdef _WIN32
#elif defined(__APPLE__)
        auto bufferSize = proc_pidinfo(getpid(), PROC_PIDLISTFDS, 0, 0, 0);
        if (bufferSize < 0) {
            return EXIT_FAILURE;
        }
        std::vector< struct proc_fdinfo > fds(bufferSize / sizeof(struct proc_fdinfo));
        bufferSize = proc_pidinfo(getpid(), PROC_PIDLISTFDS, 0, fds.data(), bufferSize);
        if (bufferSize < 0) {
            return EXIT_FAILURE;
        }
        std::ofstream report((SystemAbstractions::File::GetExeParentDirectory() + "/TestArea/handles").c_str());
        fds.resize(bufferSize / sizeof(struct proc_fdinfo));
        const auto pid = getpid();
        for (const auto& fd: fds) {
            report << fd.proc_fd << std::endl;
        }
#else /* Linux */
        std::vector< std::string > fds;
        const std::string fdsDir("/proc/self/fd/");
        SystemAbstractions::File::ListDirectory(fdsDir, fds);
        std::ostringstream report;
        std::vector< char > link(64);
        for (const auto& fd: fds) {
            if (readlink(fd.c_str(), link.data(), link.size()) >= 0) {
                report << fd << std::endl;
            }
        }
        SystemAbstractions::File handlesReport(SystemAbstractions::File::GetExeParentDirectory() + "/TestArea/handles");
        (void)handlesReport.OpenReadWrite();
        const auto reportString = report.str();
        (void)handlesReport.Write(reportString.data(), reportString.length());
#endif /* various platforms */
    } else {
        return EXIT_FAILURE;
    }
    const std::string testFilePath = SystemAbstractions::File::GetExeParentDirectory() + "/TestArea/foo.txt";
    auto f = fopen(testFilePath.c_str(), "w");
    for (const auto& arg: args) {
        (void)fprintf(f, "%s\n", arg.c_str());
    }
    (void)fclose(f);
    if (
        (args.size() >= 2)
        && (args[1] == "crash")
    ) {
        volatile int* null = (int*)0;
        *null = 0;
    } else if (
        (args.size() >= 2)
        && (args[1] == "handles")
    ) {
#ifdef _WIN32
#elif defined(__APPLE__)
        auto bufferSize = proc_pidinfo(getpid(), PROC_PIDLISTFDS, 0, 0, 0);
        if (bufferSize < 0) {
            return EXIT_FAILURE;
        }
        std::vector< struct proc_fdinfo > fds(bufferSize / sizeof(struct proc_fdinfo));
        bufferSize = proc_pidinfo(getpid(), PROC_PIDLISTFDS, 0, fds.data(), bufferSize);
        if (bufferSize < 0) {
            return EXIT_FAILURE;
        }
        std::ofstream report((SystemAbstractions::File::GetExeParentDirectory() + "/TestArea/handles").c_str());
        fds.resize(bufferSize / sizeof(struct proc_fdinfo));
        const auto pid = getpid();
        for (const auto& fd: fds) {
            if ((int)fd.proc_fd != pipeToParentFd) {
                report << fd.proc_fd << std::endl;
            }
        }
#else /* Linux */
        std::vector< std::string > fds;
        const std::string fdsDir("/proc/self/fd/");
        SystemAbstractions::File::ListDirectory(fdsDir, fds);
        std::ostringstream report;
        std::vector< char > link(64);
        for (const auto& fd: fds) {
            if (
                (fd != pipeToParentFdPath)
                && (readlink(fd.c_str(), link.data(), link.size()) >= 0)
            ) {
                report << fd << std::endl;
            }
        }
        SystemAbstractions::File handlesReport(SystemAbstractions::File::GetExeParentDirectory() + "/TestArea/handles");
        (void)handlesReport.OpenReadWrite();
        const auto reportString = report.str();
        (void)handlesReport.Write(reportString.data(), reportString.length());
#endif /* various platforms */
    }
    return EXIT_SUCCESS;
}
