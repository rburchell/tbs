#ifndef TARGET_H
#define TARGET_H

#include <vector>
#include <string>

class translation_unit
{
public:
    translation_unit(const std::string &name);
    std::string source_name() const;
    std::string object_name() const;

private:
    std::string m_name;
};

class target
{
public:
    target();
    void set_name(const std::string &name);
    std::string name() const;
    std::vector<translation_unit> translation_units() const;
    void set_translation_units(const std::vector<translation_unit> &files);

private:
    std::string m_name;
    std::vector<translation_unit> m_translation_units;
};

#endif // TARGET_H
