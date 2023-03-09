
#include "LineSpan.h"

namespace odtr {
namespace priv {

LineSpan::LineSpan(long start, long end, float lineWidth, TextDirection dir, Justifiable justifiable)
    : start(start), end(end), lineWidth(lineWidth), baseDir(dir), justifiable(justifiable)
{ }

long LineSpan::size() const {
    long size = 0;
    for (const VisualRun &vr : visualRuns) {
        size += vr.size();
    }
    return size;
}

} // namespace priv
} // namespace odtr
