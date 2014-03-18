#include "target.h"

target::target(const std::string &name)
    : m_name(name)
{
}

std::string target::name() const
{
    return m_name;
}

std::vector<std::string> target::source_files() const
{
    return m_source_files;
}

void target::set_source_files(const std::vector<std::string> &files)
{
    m_source_files = files;
}

std::vector<std::string> target::object_files() const
{
    std::vector<std::string> ofiles;
    for (std::string name : m_source_files) {
        std::string fname = name.substr(0, name.find_last_of("."));
        ofiles.push_back(fname + ".o");
    }

    return ofiles;
}



