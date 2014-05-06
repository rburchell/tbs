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

/* $Foo: 1 */
/* $Foo.Bar: 1 */
/* $Foo.Bar.Moo: 1 */
/* $Foo.Bar.Moo.Cow: 1 */

static bool keyword_search(const translation_unit &tu, std::map<std::string, std::string> &keywords)
{
    /* scan for keywords */
    char buf[LINE_MAX];
    std::string fname = tu.path() + "/" + tu.source_name();
    if (global_options::instance().debug_level() >= 2)
        DEBUG("opening %s", fname.c_str());
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

                if (global_options::instance().debug_level() >= 2)
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

bool tree_to_targets(directory_node &root, std::vector<target> &final_targets)
{
    std::queue<directory_node> q;
    std::stack<target> current_targets;

    target rt;
    rt.set_path(root.path);
    current_targets.push(rt);

    // we have a tree of directories (and files), we now need to turn this into
    // a list (maybe one day, a tree) of targets.
    //
    // the rules for this are that we create an implicit root target, and every
    // file in every subdirectory under a target goes into the current target
    // until we find a directory with an explicitly created target.
    //
    // if we find one of those, we push a new current target onto the stack and
    // repeat the process for that part of the directory tree, and so on.
    //
    // this obviously means we need to do a breadth-first traversal of the tree.
    q.push(root);

    // while there's something still in the tree..
    while (!q.empty()) {
        directory_node n = q.front();
        if (global_options::instance().debug_level() >= 2)
            DEBUG("turning %s into targets", n.path.c_str());

        // scan the TUs in this directory for keywords & potential target changes
        for (const translation_unit &tu : n.files) {
            std::map<std::string, std::string> keywords;
            if (!keyword_search(tu, keywords))
                exit(1); // TODO: refactor

            std::string name = keywords["target.name"];
            if (!name.empty()) {
                // if the TU is in the same directory as the target, alter name
                if (tu.path() == current_targets.top().path()) {
                    if (current_targets.top().explicitly_named()) {
                        WARNING("target %s already has a name, can't deal with another found in %s", current_targets.top().name().c_str(), tu.source_name().c_str());
                        exit(1);
                    } else {
                        current_targets.top().set_name(name);
                        if (global_options::instance().debug_level() >= 2)
                            DEBUG("renaming target %s to %s", current_targets.top().path().c_str(), name.c_str());
                    }
                } else {
                    // otherwise start a new target
                    target t;
                    t.set_path(tu.path());
                    t.set_name(name);
                    current_targets.push(t);
                    if (global_options::instance().debug_level() >= 2)
                        DEBUG("creating new target %s", tu.path().c_str());
                }
            }

            // TODO: this is wrong. we must do this after the target has
            // potentially been reset (below).
            std::string cflags = keywords["target.compileflags"];
            if (!cflags.empty()) {
                target &t = current_targets.top();
                if (t.compile_flags().empty()) {
                    t.set_compile_flags(cflags);
                } else {
                    t.set_compile_flags(t.compile_flags() + " " + cflags);
                }
            }
        }

        // now (and only now; as we may have created a new target), parent the
        // files to the current target & do other things that affect the target
        {
            target &t = current_targets.top();
            std::vector<translation_unit> efiles = t.translation_units();
            efiles.insert(efiles.end(), n.files.begin(), n.files.end());
            t.set_translation_units(efiles);

            if (global_options::instance().debug_level() >= 2)
                for (const translation_unit &tu : n.files) {
                    DEBUG("target %s (%s) has file %s", t.name().c_str(), t.path().c_str(), tu.source_name().c_str());
                }
        }

        if (n.children.empty()) {
            // reached the end of this tree? no more targets/changes to this target
            target &t = current_targets.top();
            if (t.path() == n.path) {
                if (global_options::instance().debug_level() >= 2)
                    DEBUG("finished target %s (%s)", t.name().c_str(), t.path().c_str());
                final_targets.push_back(t);
                current_targets.pop();
            }
        } else {
            // we have further to go in this branch... enqueue children
            for (const directory_node &c : n.children)
                q.push(c);
        }

        // pop visited
        q.pop();
    }

    // how can we possibly get more assuming a valid tree
    assert(current_targets.size() <= 1);
    if (current_targets.size())
        final_targets.push_back(current_targets.top());
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

