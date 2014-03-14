#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>

#include <map>
#include <string>
#include <vector>
#include <iterator>
#include <sstream>

#include "forkfd.h"
#include "futils.h"
#include "directory.h"

#define EINTR_LOOP(var, cmd) \
    do { \
        var = cmd; \
    } while (var == -1 && errno == EINTR)


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

    if (fd == -1)
        perror("builder::compile: forkfd");
    else if (fd == FFD_CHILD_PROCESS)
        exit(system(cmd.c_str()));

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

    int maxjobs = 2;
    std::map<int, std::string> curjobs;

    // while there are things to build
    while (ccopy.size() || curjobs.size()) {
        // check status on existing builds (if any)
        if (curjobs.size() != 0) {
            fd_set readfds;
            FD_ZERO(&readfds);

            for (auto it = curjobs.cbegin(); it != curjobs.cend(); ++it)
                FD_SET(it->first, &readfds);

            int fds = 0;

            // monitor until all compiles are done
            EINTR_LOOP(fds, select(FD_SETSIZE, &readfds, NULL, NULL, NULL));

            // close completed jobs
            for (auto it = curjobs.begin(); it != curjobs.end();) {
                if (FD_ISSET(it->first, &readfds)) {
                    siginfo_t info;
                    int retval = 0;
                    EINTR_LOOP(retval, read(it->first, &info, sizeof(info)));
                    if (retval != sizeof(info)) {
                        perror("can't get siginfo_t struct from forkfd");
                        return -1;
                    }

                    printf("fd %d (job %s) had return code %d\n", it->first, it->second.c_str(), info.si_status);

                    if (info.si_status != 0) {
                        printf("error compiling %s\n", it->second.c_str());
                        return -1;
                    }

                    EINTR_LOOP(retval, close(it->first));
                    it = curjobs.erase(it);
                } else {
                    it++;
                }
            }
        }

        // start builds
        while (curjobs.size() < maxjobs && ccopy.size()) {
            // take a job
            std::string name = ccopy.back();
            ccopy.pop_back();

            int compile_fd = builder::compile(name);
            if (compile_fd == -1) {
                printf("forkfd for compile job %s failed\n", name.c_str());
                return -1;
            }

            curjobs[compile_fd] = name;
            printf("compiling %s, compile_fd %d\n", name.c_str(), compile_fd);
        }
    }


    // TODO: optimize for the case where there's only a single file and don't
    // generate a .o

    printf("linking %s\n", target.c_str());
    bool linked_ok = builder::link(target, cfiles);
//    printf("linking of %s: %d\n", target.c_str(), linked_ok);
}
