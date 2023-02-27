
#pragma once

#include <vector>

#include "text-format.h"

namespace odtr {

struct VisualRun
{
    long start, end;
    TextDirection dir;
    float width;
};
using VisualRuns = std::vector<VisualRun>;

} // namespace odtr
