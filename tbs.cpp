#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>

#include <string>
#include <vector>
#include <iterator>
#include <sstream>

#include "futils.h"

class directory
{
public:
    directory(const char *dirname);
    ~directory();
    bool is_open() const;

    dirent *next_entry();

private:
    DIR *m_dir;
};

directory::directory(const char *dirname)
{
    m_dir = opendir(dirname);
}

directory::~directory()
{
    closedir(m_dir);
}

bool directory::is_open() const
{
    return m_dir != NULL;
}

dirent *directory::next_entry()
{
    return readdir(m_dir);
}

int main(int argc, char **argv)
{
    directory d(".");

    char targbuf[PATH_MAX];
    std::string target = futils::basename(getcwd(targbuf, PATH_MAX));

    std::vector<std::string> cppfiles;
    std::vector<std::string> cfiles;

    dirent *dnt = NULL;
    while ((dnt = d.next_entry()) != NULL) {
        const char *extension = futils::extension(dnt->d_name);
        if (extension) {
            if (strcmp(extension, "cpp") == 0)
                cppfiles.push_back(dnt->d_name);
            else if (strcmp(extension, "c") == 0)
                cfiles.push_back(dnt->d_name);
        }
    }

    std::string buildcmd;
    std::string linkcmd;

    if (cppfiles.size() == 1 && cfiles.size() == 0 ||
        cppfiles.size() == 0 && cfiles.size() == 1) {
        // special case: don't generate .o
        std::vector<std::string> params;
        params.push_back("g++");
        params.push_back("-o");
        params.push_back(target);

        // TODO: quote if we're going to use shell
        if (cppfiles.size() == 1)
            params.push_back(cppfiles[0]);
        else
        params.push_back(cfiles[0]);

        printf("compiling %s\n", cppfiles[0].c_str());
        std::stringstream ss;
        std::copy(params.begin(), params.end(), std::ostream_iterator<std::string>(ss, " "));
        std::string cmd = ss.str();

        int retval = system(cmd.c_str());
        printf("compilation of %s: %d\n", target.c_str(), retval);
        return retval;
    } else {
        // generate .o's and link after
        std::vector<std::string> params;
        params.push_back("g++");
        params.push_back("-c");

        for (std::string name : cppfiles) {
            std::vector<std::string> cparams = params;
            cparams.push_back(name);
            printf("compiling %s\n", name.c_str());

            std::stringstream ss;
            std::copy(cparams.begin(), cparams.end(), std::ostream_iterator<std::string>(ss, " "));
            std::string cmd = ss.str();

            int retval = system(cmd.c_str());
            printf("compilation of %s: %d\n", name.c_str(), retval);
        }

        params.clear();
        params.push_back("g++");
        params.push_back("-o");
        params.push_back(target);

        printf("linking %s\n", target.c_str());

        for (std::string name : cppfiles) {
            std::string fname = name.substr(0, name.find_last_of(".") /* couldn't we just hardcode the ".cpp"? */);
            params.push_back(fname + ".o");
        }
    }
}
