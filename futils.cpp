#include "futils.h"

#include <string.h>

namespace futils
{
    const char *extension(const char *fname)
    {
        const char *last_dot = strrchr(fname, '.');

        if (last_dot)
            return ++last_dot; // exclude the dot
        return last_dot;
    }

    const char *basename(const char *fname)
    {
        const char *last_slash = strrchr(fname, '/');

        if (last_slash)
            return ++last_slash; // exclude the slash
        return fname;
    }
}


