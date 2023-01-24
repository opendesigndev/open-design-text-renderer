
#pragma once

#include <memory>
#include <vector>

namespace textify {

struct PlacedDecoration {
    /// Decoration - underline, strikethrough etc.
    enum class Type {
        NONE = 0,
        UNDERLINE,
        DOUBLE_UNDERLINE,
        STRIKE_THROUGH,
    } type;

    struct {
        int first, last;
    } xRange; ///< horizontal range in pixels

    uint32_t color;
    int yOffset;
    float thickness;
};
using PlacedDecorationPtr = std::unique_ptr<PlacedDecoration>;
using PlacedDecorations = std::vector<PlacedDecorationPtr>;

} // namespace textify
