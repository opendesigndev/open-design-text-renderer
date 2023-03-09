
#include "VisualRun.h"

namespace odtr {
namespace priv {

long VisualRun::size() const {
    return start > end ? 0 : end - start;
}

} // namespace priv
} // namespace odtr
