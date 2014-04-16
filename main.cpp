/* $Target.CompileFlags: -Wall -Wshadow -Wno-variadic-macros */

#define MODULE_NAME "tbs"

#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>

#include <map>
#include <string>
#include <vector>
#include <iterator>
#include <sstream>

#include "futils.h"
#include "scanner.h"
#include "builder.h"
#include "target.h"
#include "global_options.h"
#include "tbs.h"

#define EINTR_LOOP(var, cmd) \
    do { \
        var = cmd; \
    } while (var == -1 && errno == EINTR)

int main(int argc, char **argv)
{
    if (!global_options::instance().parse(argc, argv))
        exit(EXIT_FAILURE);

    INFO("tbs v0.0.0 running with %d jobs", global_options::instance().max_jobs());

    char targbuf[PATH_MAX];
    getcwd(targbuf, PATH_MAX); // TODO: errcheck
    std::vector<target> targets = scanner::targets(targbuf);

    for (const target &t : targets) {
        DEBUG("read target %s", t.name().c_str());

        for (const translation_unit &tu : t.translation_units()) {
            DEBUG("source file %s object file %s in path %s", tu.source_name().c_str(), tu.object_name().c_str(), tu.path().c_str());
        }
    }


    for (const target &t : targets) {
        INFO("building target %s", t.name().c_str());
        std::vector<translation_unit> cfiles = t.translation_units();
        std::map<int, std::string> curjobs;

        // while there are things to build
        while (cfiles.size() || curjobs.size()) {
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

                        if (global_options::instance().debug_level() >= 3)
                            DEBUG("fd %d (job %s) had return code %d", it->first, it->second.c_str(), info.si_status);

                        if (info.si_status != 0) {
                            WARNING("error compiling %s", it->second.c_str());
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
            while (curjobs.size() < global_options::instance().max_jobs() && cfiles.size()) {
                // take a job
                translation_unit tu = cfiles.back();
                cfiles.pop_back();

                int compile_fd = builder::compile(t, tu);
                if (compile_fd == -1) {
                    WARNING("builder for compile job %s failed", tu.source_name().c_str());
                    return -1;
                }

                curjobs[compile_fd] = tu.source_name();
                if (global_options::instance().debug_level() >= 3)
                    DEBUG("compiling %s, compile_fd %d", tu.source_name().c_str(), compile_fd);
                else
                    INFO("compiling %s", tu.source_name().c_str());
            }
        }


        // TODO: optimize for the case where there's only a single file and don't
        // generate a .o

        INFO("linking %s", t.name().c_str());
        bool linked_ok = builder::link(t);
        if (!linked_ok)
            return -1;
    }
}
