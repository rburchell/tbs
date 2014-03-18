#include <vector>
#include <string>

class target;

class directory
{
public:
    directory(const char *dirname);
    ~directory();
    bool is_open() const;
    std::vector<target *> targets() const;
    std::vector<std::string> source_files() const;

    dirent *next_entry();

private:
    DIR *m_dir;
    std::vector<target *> m_targets;
};


