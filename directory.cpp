#include <stdio.h>
#include <dirent.h>

#include "directory.h"

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


