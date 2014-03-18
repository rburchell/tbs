#include "target.h"

translation_unit::translation_unit(const std::string &name)
{
    m_name = name;
}

std::string translation_unit::source_name() const
{
    return m_name;
}

std::string translation_unit::object_name() const
{
    return m_name.substr(0, m_name.find_last_of(".")) + ".o";
}

target::target()
{
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

