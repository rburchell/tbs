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

#include "target.h"
#include "scanner.h"
#include "futils.h"
#include "global_options.h"

/* $Foo: 1 */
/* $Foo.Bar: 1 */
/* $Foo.Bar.Moo: 1 */
/* $Foo.Bar.Moo.Cow: 1 */

struct keywords
{
    std::string target_name;
};

static bool keyword_search(keywords &keywords, const translation_unit &tu)
{
    /* scan for keywords */
    char buf[LINE_MAX];
    std::string fname = tu.path() + "/" + tu.source_name();
    if (global_options::instance().debug_level() >= 2)
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

                if (global_options::instance().debug_level() >= 2)
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
        fprintf(stderr, "Cannot open directory '%s': %s\n", dirname, strerror (errno));
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
                fprintf (stderr, "Path length has got too long.\n");
                return false;
            }

            if (global_options::instance().debug_level() >= 2)
                printf("scanning %s for stuff\n", path);

            directory_node new_node(path);
            if (!directory_to_tree(path, new_node)) {
                fprintf(stderr, "failed to scan %s", path);
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
                    fprintf (stderr, "Path length has got too long.\n");
                    return false;
                }

                translation_unit tu(path);
                root.files.push_back(tu);
            }
        }
    }

    if (closedir(d)) {
        fprintf(stderr, "Could not close '%s': %s\n", dirname, strerror (errno));
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
            printf("turning %s into targets\n", n.path.c_str());

        // scan the TUs in this directory for keywords & potential target changes
        for (const translation_unit &tu : n.files) {
            keywords k;
            if (!keyword_search(k, tu))
                exit(1); // TODO: refactor

            if (!k.target_name.empty()) {
                // if the TU is in the same directory as the target, alter name
                if (tu.path() == current_targets.top().path()) {
                    if (current_targets.top().explicitly_named()) {
                        printf("scanner::targets: target %s already has a name, can't deal with another found in %s\n", current_targets.top().name().c_str(), tu.source_name().c_str());
                        exit(1);
                    } else {
                        current_targets.top().set_name(k.target_name);
                        if (global_options::instance().debug_level() >= 2)
                            printf("renaming target %s to %s\n", current_targets.top().path().c_str(), k.target_name.c_str());
                    }
                } else {
                    // otherwise start a new target
                    target t;
                    t.set_path(tu.path());
                    current_targets.push(t);
                    if (global_options::instance().debug_level() >= 2)
                        printf("creating new target %s\n", tu.path().c_str());
                }
            }
        }

        // now (and only now; as we may have created a new target), parent the
        // files to the current target
        {
            target &t = current_targets.top();
            std::vector<translation_unit> efiles = t.translation_units();
            efiles.insert(efiles.end(), n.files.begin(), n.files.end());
            t.set_translation_units(efiles);

            if (global_options::instance().debug_level() >= 2)
                for (const translation_unit &tu : n.files) {
                    printf("target %s (%s) has file %s\n", t.name().c_str(), t.path().c_str(), tu.source_name().c_str());
                }
        }

        if (n.children.empty()) {
            // reached the end of this tree? no more targets/changes to this target
            target &t = current_targets.top();
            if (t.path() == n.path) {
                if (global_options::instance().debug_level() >= 2)
                    printf("finished target %s (%s)\n", t.name().c_str(), t.path().c_str());
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
    assert(current_targets.size() == 1);
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
        fprintf(stderr, "couldn't scan directory tree under %s\n", dirname);
        exit(EXIT_FAILURE);
    }

    std::vector<target> targets;
    retval = tree_to_targets(tree, targets);

    if (!retval) {
        fprintf(stderr, "couldn't make targets from tree under %s\n", dirname);
        exit(EXIT_FAILURE);
    }

    return targets;
}

