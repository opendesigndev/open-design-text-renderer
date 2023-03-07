#include "arithmetics.hpp"

namespace odtr {
namespace compat {

bool isPOT(int x) {
    return !(x&x-1);
}

int floorPOT(int x) {
    static_assert(sizeof(x) <= 4, "Add bitors");
    x |= x>>1;
    x |= x>>2;
    x |= x>>4;
    x |= x>>8;
    x |= x>>16;
    return x-(x>>1);
}

} // namespace compat
} // namespace odtr
