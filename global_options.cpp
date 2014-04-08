#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

#include "global_options.h"

global_options::global_options()
    : m_max_jobs(0)
{
}

global_options &global_options::instance()
{
    static global_options opts;
    return opts;
}

bool global_options::parse(int argc, char **argv)
{
    m_max_jobs = 2;

    while (1) {
        static struct option long_options[] =
        {
            { "jobs",  required_argument, NULL, 'j' }
        };

        int option_index = 0;
        int c = getopt_long (argc, argv, ":j::", long_options, &option_index);

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
        }
    }

    return true;
}

int global_options::max_jobs() const
{
    return m_max_jobs;
}

