#ifndef GLOBAL_OPTIONS_H
#define GLOBAL_OPTIONS_H

/* stuff set up from the command line */
class global_options
{
public:
    static global_options &instance();

    bool parse(int argc, char **argv);

    int max_jobs() const;

    int debug_level() const;

private:
    global_options();

    int m_max_jobs;

    int m_debug_level;
};

#endif // GLOBAL_OPTIONS_H
