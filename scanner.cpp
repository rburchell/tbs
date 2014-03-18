#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <regex>

#include "target.h"
#include "scanner.h"
#include "futils.h"

scanner::scanner(const char *dirname)
{
    m_dir = opendir(dirname);
}

scanner::~scanner()
{
    closedir(m_dir);
}

bool scanner::is_open() const
{
    return m_dir != NULL;
}

dirent *scanner::next_entry()
{
    return readdir(m_dir);
}

std::vector<target> scanner::targets() const
{
    char targbuf[PATH_MAX];
    std::string tname = futils::basename(getcwd(targbuf, PATH_MAX));
    std::vector<target> targs(1);
    target &t = targs[0];
    std::vector<translation_unit> cfiles;

    bool has_target = false;

    dirent *dnt = NULL;
    while ((dnt = const_cast<scanner *>(this)->next_entry()) != NULL) {
        const char *extension = futils::extension(dnt->d_name);
        if (extension) {
            if (strcmp(extension, "cpp") != 0 &&
                strcmp(extension, "c") != 0) {
                continue;
            }

            cfiles.push_back(translation_unit(dnt->d_name));

            /* scan for keywords */
            char buf[LINE_MAX];
            FILE *fd = fopen(dnt->d_name, "r");
            if (!fd) {
                perror("scanner::targets: can't open for keyword scan");
                continue;
            }

            while (fgets(buf, LINE_MAX, fd) != NULL) {
                buf[strlen(buf) - 1] = '\0';

                std::regex pieces_regex("^\\/\\* \\$([a-Z]+): (.+) \\*\\/$", std::regex_constants::icase);
                std::smatch pieces_match;

                if (std::regex_match(std::string(buf), pieces_match, pieces_regex, std::regex_constants::match_any)) {
                    std::cout << buf << '\n';
                    if (pieces_match.size() >= 2) {
                        if (strcasecmp(pieces_match[1].str().c_str(), "target") == 0) {
                            if (has_target) {
                                printf("scanner::targets: target %s already has a target, can't deal with another found in %s\n", tname.c_str(), dnt->d_name);
                                exit(1);
                            } else {
                                tname = pieces_match[2].str();
                                has_target = true;
                            }
                        }
                    }
                    /*
                    for (size_t i = 0; i < pieces_match.size(); ++i) {
                        std::ssub_match sub_match = pieces_match[i];
                        std::string piece = sub_match.str();
                        std::cout << "  submatch " << i << ": " << piece << '\n';
                    }*/
                }
            }

            if (ferror(fd)) {
                perror("scanner::targets: error during fgets for keyword scan");
            }

            fclose(fd);
        }
    }

    t.set_name(tname);
    t.set_translation_units(cfiles);
    return targs;
}

