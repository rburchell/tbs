#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <string>

#include "target.h"
#include "directory.h"
#include "futils.h"

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

std::vector<target *> directory::targets() const
{
    char targbuf[PATH_MAX];
    std::string tname = futils::basename(getcwd(targbuf, PATH_MAX));
    target *t = new target(tname); // TODO: leak
    std::vector<std::string> cfiles;

    dirent *dnt = NULL;
    while ((dnt = const_cast<directory *>(this)->next_entry()) != NULL) {
        const char *extension = futils::extension(dnt->d_name);
        if (extension) {
            if (strcmp(extension, "cpp") == 0 ||
                strcmp(extension, "c") == 0)
                cfiles.push_back(dnt->d_name);
        }
    }

    t->set_source_files(cfiles);
    std::vector<target *> targs;
    targs.push_back(t);
    return targs;
}

