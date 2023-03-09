
#pragma once

#include <vector>

#include "text-format.h"

namespace odtr {
namespace priv {

struct VisualRun
{
    long start, end;
    TextDirection dir;
    float width;

    /// Visual run size - glyphs count in visual run
    long size() const;
};
using VisualRuns = std::vector<VisualRun>;

} // namespace priv
} // namespace odtr
