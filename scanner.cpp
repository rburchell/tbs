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

struct keywords
{
    std::string target_name;
};

static bool keyword_search(keywords &keywords, translation_unit &tu)
{
    /* scan for keywords */
    char buf[LINE_MAX];
    std::string fname = tu.path() + "/" + tu.source_name();
    printf("opening %s\n", fname.c_str());
    FILE *fd = fopen(fname.c_str(), "r");
    if (!fd) {
        perror("scanner::targets: can't open for keyword scan");
        return false;
    }

    while (fgets(buf, LINE_MAX, fd) != NULL) {
        buf[strlen(buf) - 1] = '\0';

        std::regex pieces_regex("^\\/\\* \\$([a-Z.]+): (.+) \\*\\/$", std::regex_constants::icase);
        std::smatch pieces_match;

        std::string tbuf(buf);

        if (std::regex_match(tbuf, pieces_match, pieces_regex, std::regex_constants::match_any)) {
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
                            keywords.target_name = value;
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

/* assumption: dirname is an absolute path to the directory to retrieve targets
 * from
 */
std::vector<target> scanner::targets(const char *dirname)
{
    std::vector<target> targets;
    std::vector<translation_unit> cfiles;
    std::string tname;

    DIR *d = opendir(dirname);

    if (!d) {
        fprintf(stderr, "Cannot open directory '%s': %s\n", dirname, strerror (errno));
        exit(EXIT_FAILURE);
    }

    struct dirent *dnt;

    while ((dnt = readdir(d)) != NULL) {
        if (dnt->d_type & DT_DIR) {
            /* recurse, if it isn't us or the parent */
            if (strcmp(dnt->d_name, "..") == 0 ||
                strcmp(dnt->d_name, ".") == 0)
                continue;

            int path_length;
            char path[PATH_MAX];

            path_length = snprintf (path, PATH_MAX, "%s/%s", dirname, dnt->d_name);
            if (path_length >= PATH_MAX) {
                fprintf (stderr, "Path length has got too long.\n");
                exit (EXIT_FAILURE);
            }

            printf("scanning %s for targets\n", path);
            std::vector<target> child_targets = scanner::targets(path);
            for (const target &t : child_targets) {
                targets.push_back(t);
            }
        } else {
            const char *extension = futils::extension(dnt->d_name);
            if (extension) {
                if (strcmp(extension, "cpp") != 0 &&
                    strcmp(extension, "c") != 0) {
                    continue;
                }

                int path_length;
                char path[PATH_MAX];

                path_length = snprintf (path, PATH_MAX, "%s/%s", dirname, dnt->d_name);
                if (path_length >= PATH_MAX) {
                    fprintf (stderr, "Path length has got too long.\n");
                    exit (EXIT_FAILURE);
                }

                translation_unit tu(path);
                keywords k;
                if (!keyword_search(k, tu))
                    exit(1); // TODO: refactor

                if (!k.target_name.empty()) {
                    if (!tname.empty()) {
                        printf("scanner::targets: target %s already has a target, can't deal with another found in %s\n", tname.c_str(), tu.source_name().c_str());
                        exit(1);
                    } else {
                        tname = k.target_name;
                    }
                }

                cfiles.push_back(tu);
            }
        }
    }

    if (closedir(d)) {
        fprintf(stderr, "Could not close '%s': %s\n", dirname, strerror (errno));
        exit(EXIT_FAILURE);
    }

    if (cfiles.size()) {
        target current_target;
        if (tname.empty())
            tname = futils::basename(dirname);
        current_target.set_path(dirname);
        current_target.set_name(tname);
        current_target.set_translation_units(cfiles);
        targets.push_back(current_target); // TODO: suboptimal
        printf("got current target %s\n", tname.c_str());
    }
    return targets;
}

