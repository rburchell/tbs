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

static bool keyword_search(target &target, translation_unit &tu)
{
    /* scan for keywords */
    char buf[LINE_MAX];
    FILE *fd = fopen(tu.source_name().c_str(), "r");
    if (!fd) {
        perror("scanner::targets: can't open for keyword scan");
        return false;
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
                            if (!target.name().empty()) {
                                printf("scanner::targets: target %s already has a target, can't deal with another found in %s\n", target.name().c_str(), tu.source_name().c_str());
                                return false;
                            } else {
                                target.set_name(value);
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
        return false;
    }

    fclose(fd);
    return true;
}

std::vector<target> scanner::targets(const char *dirname)
{
    char targbuf[PATH_MAX];
    std::string tname = futils::basename(getcwd(targbuf, PATH_MAX));
    std::vector<target> targs(1);
    target &t = targs[0];
    std::vector<translation_unit> cfiles;

    dirent *dnt = NULL;
    DIR *m_dir = opendir(dirname);
    while ((dnt = readdir(m_dir)) != NULL) {
        const char *extension = futils::extension(dnt->d_name);
        if (extension) {
            if (strcmp(extension, "cpp") != 0 &&
                strcmp(extension, "c") != 0) {
                continue;
            }

            char cpath[PATH_MAX];
            getcwd(cpath, PATH_MAX); // TODO: won't always be the case?
            std::string path = std::string(cpath) + "/" + dnt->d_name;
            translation_unit tu(path);
            if (!keyword_search(t, tu))
                exit(1); // TODO: refactor
            cfiles.push_back(tu);
        }
    }
    closedir(m_dir);

    if (t.name().empty())
        t.set_name(tname);
    t.set_translation_units(cfiles);
    return targs;
}

