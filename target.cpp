#include <assert.h>

#include "target.h"
#include "futils.h"

translation_unit::translation_unit(const std::string &path)
{
    m_path = path;
}

std::string translation_unit::path() const
{
    //printf("path %s\n", m_path.substr(0, m_path.find_last_of("/")).c_str());
    return m_path.substr(0, m_path.find_last_of("/"));
}

std::string translation_unit::source_name() const
{
    return futils::basename(m_path.c_str());
}

std::string translation_unit::object_name() const
{
    //printf("for %s\n", m_path.c_str());
    std::string basename = futils::basename(m_path.c_str());
    //printf("got %s\n", basename.c_str());
    return basename.substr(0, basename.find_last_of(".")) + ".o";
}

target::target()
{
}

void target::set_path(const std::string &path)
{
    m_path = path;
}

std::string target::path() const
{
    return m_path;
}

void target::set_name(const std::string &name)
{
    m_name = name;
}

std::string target::name() const
{
    return m_name;
}

std::vector<translation_unit> target::translation_units() const
{
    return m_translation_units;
}

void target::set_translation_units(const std::vector<translation_unit> &files)
{
    m_translation_units = files;
}

