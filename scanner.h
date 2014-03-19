#include <vector>
#include <string>

class translation_unit;
class target;

class scanner
{
public:
    scanner(const char *dirname);
    ~scanner();
    bool is_open() const;
    std::vector<target> targets() const;
    std::vector<std::string> source_files() const;

    dirent *next_entry();

private:
    static bool keyword_search(target &target, translation_unit &tu);

    DIR *m_dir;
};


