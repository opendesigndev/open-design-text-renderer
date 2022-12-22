
#include "LineSpan.h"

namespace textify {
namespace priv {

LineSpan::LineSpan(long start, long end, float lineWidth, TextDirection dir, Justifiable justifiable)
    : start(start), end(end), lineWidth(lineWidth), baseDir(dir), justifiable(justifiable)
{ }

} // namespace priv
} // namespace textify
