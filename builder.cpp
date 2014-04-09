#include <unistd.h>
#include <sys/stat.h>

#include <vector>
#include <sstream>
#include <string>

#include "target.h"
#include "futils.h"
#include "forkfd.h"
#include "builder.h"

int builder::compile(const target &t, const translation_unit &tu)
{
    std::vector<std::string> params;
    const char *extension = futils::extension(tu.source_name().c_str());

    if (strcmp(extension, "cpp") == 0)
        params.push_back("g++");
    else if (strcmp(extension, "c") == 0)
        params.push_back("gcc");

    params.push_back("-c");
    params.push_back(tu.source_name());
    params.push_back("-o");
    params.push_back(".obj/" + tu.object_name());
    params.push_back(t.compile_flags());

    std::stringstream ss;
    std::copy(params.begin(), params.end(), std::ostream_iterator<std::string>(ss, " "));
    std::string cmd = ss.str();

    pid_t pid;
    int fd = forkfd(FFD_CLOEXEC | FFD_NONBLOCK, &pid);

    if (fd == -1)
        perror("builder::compile: forkfd");
    else if (fd == FFD_CHILD_PROCESS) {
        if (chdir(tu.path().c_str()) == -1) {
            perror("builder::compile: chdir");
            return -1;
        }

        if (mkdir(".obj", 0777) == -1) {
            if (errno != EEXIST) {
                perror("builder::compile: mkdir");
                return -1;
            }
        }

        exit(system(cmd.c_str()));
    }

    return fd;
}

bool builder::link(const target &target)
{
    std::vector<std::string> params;
    params.push_back("g++");
    params.push_back("-o");
    params.push_back(target.name());

    for (const translation_unit &tu : target.translation_units()) {
        params.push_back(tu.path() + "/.obj/" + tu.object_name());
    }

    std::stringstream ss;
    std::copy(params.begin(), params.end(), std::ostream_iterator<std::string>(ss, " "));
    std::string cmd = ss.str();

    if (chdir(target.path().c_str()) == -1) {
        perror("builder::link: chdir");
        return -1;
    }

    int retval = system(cmd.c_str());
    return retval == 0;
}


