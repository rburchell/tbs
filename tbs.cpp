#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>

#include <string>
#include <vector>
#include <iterator>
#include <sstream>

#include "futils.h"
#include "directory.h"

namespace builder
{
    bool compile(const std::string &file);
    bool link(const std::string &target, const std::vector<std::string> &cfiles);
}

bool builder::compile(const std::string &file)
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

    int retval = system(cmd.c_str());
    return retval == 0;
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

    std::vector<std::string> cfiles;

    dirent *dnt = NULL;
    while ((dnt = d.next_entry()) != NULL) {
        const char *extension = futils::extension(dnt->d_name);
        if (extension) {
            if (strcmp(extension, "cpp") == 0 ||
                strcmp(extension, "c") == 0)
                cfiles.push_back(dnt->d_name);
        }
    }

    // TODO: optimize for the case where there's only a single file and don't
    // generate a .o

    // generate .o's and link after
    for (std::string name : cfiles) {
        printf("compiling %s\n", name.c_str());
        bool compiled_ok = builder::compile(name);
//        printf("compilation of %s: %d\n", name.c_str(), compiled_ok);
    }

    printf("linking %s\n", target.c_str());
    bool linked_ok = builder::link(target, cfiles);
//    printf("linking of %s: %d\n", target.c_str(), linked_ok);
}
