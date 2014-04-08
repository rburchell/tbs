#!/bin/sh
g++ -O0 -ggdb -o tbs builder.cpp forkfd.c futils.cpp scanner.cpp target.cpp tbs.cpp global_options.cpp
