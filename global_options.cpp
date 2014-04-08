#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

#include <thread>

#include "global_options.h"

global_options::global_options()
    : m_max_jobs(std::thread::hardware_concurrency())
    , m_debug_level(0)
{
    if (m_max_jobs == 0) {
        fprintf(stderr, "global_options::global_options: can't determine std::thread::hardware_concurrency, guessing 2");
        m_max_jobs = 2;
    }
}

global_options &global_options::instance()
{
    static global_options opts;
    return opts;
}

bool global_options::parse(int argc, char **argv)
{
    while (1) {
        static struct option long_options[] =
        {
            { "jobs",   required_argument, NULL, 'j' },
            { "debug",  no_argument,       NULL, 'd' }
        };

        int option_index = 0;
        int c = getopt_long (argc, argv, ":jd::", long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {
            case 'j':
                if (!optarg) {
                    fprintf(stderr, "-j requires a number of jobs\n");
                    return false;
                }

                m_max_jobs = atoi(optarg);
                break;
            case 'd':
                m_debug_level++;
                break;
        }
    }

    return true;
}

int global_options::max_jobs() const
{
    return m_max_jobs;
}

int global_options::debug_level() const
{
    return m_debug_level;
}
