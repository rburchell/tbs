#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>

#include <string>
#include <vector>
#include <iterator>
#include <sstream>

#include "forkfd.h"
#include "futils.h"
#include "directory.h"

namespace builder
{
    int compile(const std::string &file);
    bool link(const std::string &target, const std::vector<std::string> &cfiles);
}

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

    if (fd == FFD_CHILD_PROCESS) {
        int retval = system(cmd.c_str());
        exit(0);
    }
    return fd;
}

bool builder::link(const std::string &target, const std::vector<std::string> &cfiles)
{
    std::vector<std::string> params;
    params.push_back("g++");
    params.push_back("-o");
    params.push_back(target);

    for (std::string name : cfiles) {
        std::string fname = name.substr(0, name.find_last_of("."));
        params.push_back(fname + ".o");
    }

    std::stringstream ss;
    std::copy(params.begin(), params.end(), std::ostream_iterator<std::string>(ss, " "));
    std::string cmd = ss.str();

    int retval = system(cmd.c_str());
    return retval == 0;
}

int main(int argc, char **argv)
{
    directory d(".");

    char targbuf[PATH_MAX];
    std::string target = futils::basename(getcwd(targbuf, PATH_MAX));

    std::vector<std::string> cfiles = d.source_files();
    std::vector<std::string> ccopy = cfiles;

    // TODO: optimize for the case where there's only a single file and don't
    // generate a .o
    while (ccopy.size()) {
        std::vector<int> cfds;
        fd_set readfds;

        FD_ZERO(&readfds);

        int spawnjobs = std::min(2 /* TODO: hardcoding */, (int)ccopy.size());

        for (int i = 0; i < spawnjobs; ++i) {
            std::string name = ccopy[i];
            int compile_fd = builder::compile(name);
            printf("compiling %s, job %d, compile_fd %d\n", name.c_str(), i, compile_fd);
            // TODO: error check compile_fd
            FD_SET(compile_fd, &readfds);
            printf("compilation of %s: %d\n", name.c_str(), compile_fd);
        }

        ccopy.erase(ccopy.begin(), ccopy.begin() + spawnjobs);

        int fds = 0;

        // monitor until all compiles are done
        do {
            fd_set read_copy = readfds;
            fds = select(FD_SETSIZE, &read_copy, NULL, NULL, NULL);
//            printf("%d compiles finished, want %d, err %s\n", fds, cfiles.size(), strerror(errno));
        } while (fds != spawnjobs);

        // close completed jobs
        // TODO: start new jobs as old ones finish
        for (int i = 0; i < FD_SETSIZE; ++i) {
            if (FD_ISSET(i, &readfds))
                close(i);
        }
    }

    printf("linking %s\n", target.c_str());
    bool linked_ok = builder::link(target, cfiles);
//    printf("linking of %s: %d\n", target.c_str(), linked_ok);
}
