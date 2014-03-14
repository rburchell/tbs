class directory
{
public:
    directory(const char *dirname);
    ~directory();
    bool is_open() const;

    dirent *next_entry();

private:
    DIR *m_dir;
};


