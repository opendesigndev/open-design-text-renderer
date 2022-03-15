#pragma once

namespace textify {
namespace compat {

template <typename T>
T clamp(T x, T a, T b) {
    if (x < a)
        return a;
    if (x > b)
        return b;
    return x;
}

/// Is integer a power of two?
bool isPOT(int x);
/// Round to the closest lower power of two
int floorPOT(int x);

} // namespace compat
} // namespace textify
