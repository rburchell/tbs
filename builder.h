#ifndef BUILDER_H
#define BUILDER_H

#include "target.h"

namespace builder
{
    int compile(const target &t, const translation_unit &tu);
    bool link(const target &target);
}

#endif // BUILDER_H
