/*
 * Copyright (c) 2014 Robin Burchell <robin+git@viroteck.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
