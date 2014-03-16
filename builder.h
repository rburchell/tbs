namespace builder
{
    int compile(const std::string &file);
    bool link(const std::string &target, const std::vector<std::string> &cfiles);
}

