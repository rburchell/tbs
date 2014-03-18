class target;

namespace builder
{
    int compile(const std::string &file);
    bool link(target *target);
}

