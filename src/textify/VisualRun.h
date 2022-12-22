
#pragma once

#include <vector>

#include "text-format.h"

namespace textify {

struct VisualRun
{
    long start, end;
    TextDirection dir;
    float width;
};
using VisualRuns = std::vector<VisualRun>;

} // namespace textify
