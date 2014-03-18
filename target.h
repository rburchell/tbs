#include <vector>
#include <string>

class target
{
public:
    target(const std::string &name);
    std::string name() const;
    std::vector<std::string> source_files() const;
    void set_source_files(const std::vector<std::string> &files);
    std::vector<std::string> object_files() const;
private:
    std::string m_name;
    std::vector<std::string> m_source_files;
};


