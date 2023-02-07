
#pragma once

#include <memory>
#include <vector>

namespace textify {

struct Vector2f_ {
    float x, y;
};

/// A single glyph decoration (underline / strikethrough) which is a result of the shaping phase, placed into its containing layer.
struct PlacedDecoration {
    /// Decoration type - underline, double underline or strikethrough.
    enum class Type {
        NONE = 0,
        UNDERLINE,
        DOUBLE_UNDERLINE,
        STRIKE_THROUGH,
    } type;
    /// Decoration placement in the containing layer, specified by its four quad corners.
    struct QuadCorners {
        Vector2f_ topLeft;
        Vector2f_ topRight;
        Vector2f_ bottomLeft;
        Vector2f_ bottomRight;
    } placement;
    /// Decoration color (incl. alpha).
    uint32_t color;
};
using PlacedDecorationPtr = std::unique_ptr<PlacedDecoration>;
using PlacedDecorations = std::vector<PlacedDecorationPtr>;

} // namespace textify
