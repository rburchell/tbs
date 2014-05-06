#ifndef TARGET_H
#define TARGET_H

#include <vector>
#include <string>

class translation_unit
{
public:
    translation_unit(const std::string &path);
    std::string path() const;
    std::string source_name() const;
    std::string object_name() const;

private:
    std::string m_path;
};

class target
{
public:
    target();

    void set_path(const std::string &path);
    std::string path() const;

    void set_name(const std::string &name);
    std::string name() const;
    bool explicitly_named() const; /* was set_name invoked? */

    std::vector<translation_unit> translation_units() const;
    void set_translation_units(const std::vector<translation_unit> &files);

    std::string compile_flags() const;
    void set_compile_flags(const std::string &flags);

    bool has_feature(const std::string &feature) const;
    void set_features(const std::string &features);

private:
    std::string m_name;
    bool m_explicitly_named;
    std::string m_path;
    std::string m_compile_flags;
    std::string m_features;
    std::vector<translation_unit> m_translation_units;
};

#endif // TARGET_H
