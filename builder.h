#ifndef BUILDER_H
#define BUILDER_H

#include "target.h"

namespace builder
{
    int compile(const translation_unit &tu);
    bool link(target *target);
}

#endif // BUILDER_H
