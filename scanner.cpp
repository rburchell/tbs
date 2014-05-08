#define MODULE_NAME "scanner"

#include <stdio.h>
#include <dirent.h>
#include <assert.h>
#include <unistd.h>

#include <sstream>
#include <algorithm>
#include <iostream>
#include <string>
#include <regex>
#include <queue>
#include <stack>
#include <map>

#include "tbs.h"
#include "target.h"
#include "scanner.h"
#include "futils.h"
#include "global_options.h"

static bool keyword_search(const translation_unit &tu, std::map<std::string, std::string> &keywords)
{
    /* scan for keywords */
    char buf[LINE_MAX];
    std::string fname = tu.path() + "/" + tu.source_name();
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
            if (pieces_match.size() >= 2) {
                // first we lowercase
                std::string key = pieces_match[1].str();
                std::string value = pieces_match[2].str();

                // lowercase
                std::transform(key.begin(), key.end(), key.begin(), ::tolower);

                if (global_options::instance().debug_level() >= 4)
                    DEBUG("scanner::target: %s val %s", key.c_str(), value.c_str());

                keywords[key] = value;
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

struct directory_node
{
    std::string path;
    std::vector<directory_node> children;
    std::vector<translation_unit> files;

    directory_node(const std::string &p)
        : path(p)
    {
    }
};

// converts a path into a map of dir => translation_unit maps
// the map can then be scanned to create targets
bool directory_to_tree(const char *dirname, directory_node &root)
{
    DIR *d = opendir(dirname);

    if (!d) {
        WARNING("cannot open directory '%s': %s", dirname, strerror (errno));
        return false;
    }

    struct dirent *dnt;

    while ((dnt = readdir(d)) != NULL) {
        if (dnt->d_type & DT_DIR) {
            /* recurse, if it isn't a dot-dir */
            if (dnt->d_name[0] == '.')
                continue;

            int path_length;
            char path[PATH_MAX];

            path_length = snprintf (path, PATH_MAX, "%s/%s", dirname, dnt->d_name);
            if (path_length >= PATH_MAX) {
                WARNING("Path length has got too long.");
                return false;
            }

            if (global_options::instance().debug_level() >= 2)
                DEBUG("scanning %s for stuff", path);

            directory_node new_node(path);
            if (!directory_to_tree(path, new_node)) {
                WARNING("failed to scan %s", path);
                return false;
            }
            root.children.push_back(new_node);
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
                    WARNING("Path length has got too long.");
                    return false;
                }

                translation_unit tu(path);
                root.files.push_back(tu);
            }
        }
    }

    if (closedir(d)) {
        WARNING("Could not close '%s': %s", dirname, strerror (errno));
        return false;
    }

    return true;
}

bool tree_to_targets(const directory_node &root, std::vector<target> &final_targets)
{
    static std::stack<target> unfinished_targets;

    DEBUG("STARTING SUBDIRECTORY %s", root.path.c_str());
    if (unfinished_targets.empty()) {
        // first call, initialize the stack
        target t;
        t.set_path(root.path);
        unfinished_targets.push(t);
        DEBUG("STARTED INITIAL target %s (%s)", t.name().c_str(), t.path().c_str());
    }

    // find a name for our target
    for (const translation_unit &tu : root.files) {
        std::map<std::string, std::string> keywords;
        if (!keyword_search(tu, keywords))
            return false;

        std::string name = keywords["target.name"];
        if (!name.empty()) {
            // if the TU is in the same directory as the target, alter name
            if (tu.path() == unfinished_targets.top().path()) {
                if (unfinished_targets.top().explicitly_named()) {
                    WARNING("target %s already has a name, can't deal with another found in %s", unfinished_targets.top().name().c_str(), tu.source_name().c_str());
                    return false;
                } else {
                    unfinished_targets.top().set_name(name);
                    if (global_options::instance().debug_level() >= 2)
                        DEBUG("renaming target %s to %s", unfinished_targets.top().path().c_str(), name.c_str());
                }
            } else {
                if (global_options::instance().debug_level() >= 2)
                    DEBUG("STARTING new target %s (%s)", name.c_str(), tu.path().c_str());
                // otherwise start a new target
                target nt;
                nt.set_path(tu.path());
                nt.set_name(name);
                unfinished_targets.push(nt);
            }
        }
    }

    // make sure the target has a name
    // this will only apply for the first (implicitly created) target
    if (!unfinished_targets.top().explicitly_named()) {
        WARNING("target %s must be explicitly named!", unfinished_targets.top().name().c_str());
        return false;
    }

    // now we have a final target set for the translation units, scan the
    // files for keywords to set on this target
    for (const translation_unit &tu : root.files) {
        std::map<std::string, std::string> keywords;
        if (!keyword_search(tu, keywords))
            return false;

        std::string cflags = keywords["target.compilerflags"];
        if (!cflags.empty()) {
            if (unfinished_targets.top().compiler_flags().empty()) {
                unfinished_targets.top().set_compiler_flags(cflags);
            } else {
                unfinished_targets.top().set_compiler_flags(unfinished_targets.top().compiler_flags() + " " + cflags);
            }
        }

        std::string lflags = keywords["target.linkerflags"];
        if (!lflags.empty()) {
            if (unfinished_targets.top().linker_flags().empty()) {
                unfinished_targets.top().set_linker_flags(lflags);
            } else {
                unfinished_targets.top().set_linker_flags(unfinished_targets.top().linker_flags() + " " + lflags);
            }
        }

        std::string features = keywords["target.features"];
        if (!features.empty()) {
            unfinished_targets.top().set_features(features);
        }

        std::string type = keywords["target.type"];
        if (!type.empty()) {
            if (type != "app" &&
                type != "dll") {
                    WARNING("target %s has a bad type (%s) in %s", unfinished_targets.top().name().c_str(), type.c_str(), tu.source_name().c_str());
                    return false;
            }

            if (type == "app")
                unfinished_targets.top().set_type(target::TYPE_APPLICATION);
            else if (type == "dll")
                unfinished_targets.top().set_type(target::TYPE_DLL);
        }
    }

    // add translation units to target
    std::vector<translation_unit> efiles = unfinished_targets.top().translation_units();
    efiles.insert(efiles.end(), root.files.begin(), root.files.end());
    unfinished_targets.top().set_translation_units(efiles);

    if (global_options::instance().debug_level() >= 2)
        for (const translation_unit &tu : root.files) {
            DEBUG("target %s (%s) has file %s", unfinished_targets.top().name().c_str(), unfinished_targets.top().path().c_str(), tu.source_name().c_str());
        }

    // recurse
    for (const directory_node &c : root.children) {
        bool r = tree_to_targets(c, final_targets);
        if (!r)
            return false;
    }

    if (unfinished_targets.top().path() == root.path) {
        // finished recursing for this target
        DEBUG("FINISHING new target %s (%s)", unfinished_targets.top().name().c_str(), unfinished_targets.top().path().c_str());
        final_targets.push_back(unfinished_targets.top());
        unfinished_targets.pop();
        if (unfinished_targets.size()) {
            DEBUG("NOW WANT %s", unfinished_targets.top().path().c_str());
        } else {
            DEBUG("DONE???");
        }
    } else {
        DEBUG("FINISHED SUBDIRECTORY %s WANT %s", root.path.c_str(), unfinished_targets.top().path().c_str());
    }

    return true;
}

/* assumption: dirname is an absolute path to the directory to retrieve targets
 * from
 */
std::vector<target> scanner::targets(const char *dirname)
{
    directory_node tree(dirname);
    bool retval = directory_to_tree(dirname, tree);

    if (!retval) {
        WARNING("couldn't scan directory tree under %s", dirname);
        exit(EXIT_FAILURE);
    }

    std::vector<target> targets;
    retval = tree_to_targets(tree, targets);

    if (!retval) {
        WARNING("couldn't make targets from tree under %s", dirname);
        exit(EXIT_FAILURE);
    }

    return targets;
}

