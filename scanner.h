#include <vector>
#include <string>

class target;

class scanner
{
public:
    scanner(const char *dirname);
    ~scanner();
    bool is_open() const;
    std::vector<target *> targets() const;
    std::vector<std::string> source_files() const;

    dirent *next_entry();

private:
    DIR *m_dir;
    std::vector<target *> m_targets;
};


