#include <vector>

class directory
{
public:
    directory(const char *dirname);
    ~directory();
    bool is_open() const;
    std::vector<std::string> source_files() const;

    dirent *next_entry();

private:
    DIR *m_dir;
};


