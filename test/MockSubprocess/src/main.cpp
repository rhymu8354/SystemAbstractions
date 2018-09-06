#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <SystemAbstractions/File.hpp>
#include <SystemAbstractions/Subprocess.hpp>

int main(int argc, char* argv[]) {
    SystemAbstractions::Subprocess parent;
    std::vector< std::string > args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }
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
    }
    return EXIT_SUCCESS;
}
