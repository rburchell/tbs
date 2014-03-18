#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <sstream>
#include <algorithm>
#include <iostream>
#include <string>
#include <regex>

#include "target.h"
#include "scanner.h"
#include "futils.h"

/* $Foo: 1 */
/* $Foo.Bar: 1 */
/* $Foo.Bar.Moo: 1 */
/* $Foo.Bar.Moo.Cow: 1 */

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

                std::regex pieces_regex("^\\/\\* \\$([a-Z.]+): (.+) \\*\\/$", std::regex_constants::icase);
                std::smatch pieces_match;

                if (std::regex_match(std::string(buf), pieces_match, pieces_regex, std::regex_constants::match_any)) {
                    std::cout << buf << '\n';
                    if (pieces_match.size() >= 2) {
                        // first we lowercase
                        std::string key = pieces_match[1].str();
                        std::string value = pieces_match[2].str();

                        // lowercase
                        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

                        // now split
                        std::vector<std::string> keybits;

                        std::istringstream iss(key);
                        std::string token;
                        while (std::getline(iss, token, '.'))
                            keybits.push_back(token);

                        printf("scanner::target: %s (%d bits) val %s\n", key.c_str(), keybits.size(), value.c_str());

                        if (keybits.size() == 2) {
                            if (keybits[0] == "target") {
                                if (keybits[1] == "name") {
                                    if (has_target) {
                                        printf("scanner::targets: target %s already has a target, can't deal with another found in %s\n", tname.c_str(), dnt->d_name);
                                        exit(1);
                                    } else {
                                        tname = value;
                                        has_target = true;
                                    }
                                }
                            }
                        }
                    }
#if 0
                    for (size_t i = 0; i < pieces_match.size(); ++i) {
                        std::ssub_match sub_match = pieces_match[i];
                        std::string piece = sub_match.str();
                        std::cout << "  submatch " << i << ": " << piece << '\n';
                    }
#endif
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

