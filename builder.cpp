#include <vector>
#include <sstream>
#include <string>

#include "target.h"
#include "futils.h"
#include "forkfd.h"
#include "builder.h"

int builder::compile(const std::string &file)
{
    std::vector<std::string> params;
    const char *extension = futils::extension(file.c_str());

    if (strcmp(extension, "cpp") == 0)
        params.push_back("g++");
    else if (strcmp(extension, "c") == 0)
        params.push_back("gcc");

    params.push_back("-c");

    params.push_back(file);

    std::stringstream ss;
    std::copy(params.begin(), params.end(), std::ostream_iterator<std::string>(ss, " "));
    std::string cmd = ss.str();

    pid_t pid;
    int fd = forkfd(FFD_CLOEXEC | FFD_NONBLOCK, &pid);

    if (fd == -1)
        perror("builder::compile: forkfd");
    else if (fd == FFD_CHILD_PROCESS)
        exit(system(cmd.c_str()));

    return fd;
}

bool builder::link(target *target)
{
    std::vector<std::string> params;
    params.push_back("g++");
    params.push_back("-o");
    params.push_back(target->name());

    std::vector<std::string> ofiles = target->object_files();
    params.insert(params.end(), ofiles.begin(), ofiles.end());

    std::stringstream ss;
    std::copy(params.begin(), params.end(), std::ostream_iterator<std::string>(ss, " "));
    std::string cmd = ss.str();

    int retval = system(cmd.c_str());
    return retval == 0;
}


