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

int main(int argc, char* argv[]) {
    SystemAbstractions::Subprocess parent;
    std::vector< std::string > args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }
#ifndef _WIN32
    const std::string pipeToParentFdPath = "/proc/self/fd/" + args[1];
#endif /* not _WIN32 */
    if (!parent.ContactParent(args)) {
        return EXIT_FAILURE;
    }
    const std::string testFilePath = SystemAbstractions::File::GetExeParentDirectory() + "/TestArea/foo.txt";
    auto f = fopen(testFilePath.c_str(), "wt");
    const std::string testLine = args[0] + "\n";
    (void)fputs(testLine.c_str(), f);
    (void)fclose(f);
    if (args[1] == "crash") {
        volatile int* null = (int*)0;
        *null = 0;
#ifndef _WIN32
    } else if (args[1] == "handles") {
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
        (void)handlesReport.Create();
        const auto reportString = report.str();
        (void)handlesReport.Write(reportString.data(), reportString.length());
#endif /* not _WIN32 */
    }
    return EXIT_SUCCESS;
}
