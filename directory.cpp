#include <stdio.h>
#include <dirent.h>

#include <string>

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

std::vector<std::string> directory::source_files() const
{
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

    return cfiles;
}

