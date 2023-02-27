
#pragma once

#include <memory>
#include <vector>

#include "textify-api.h"

namespace odtr {

/// A single glyph decoration (underline / strikethrough) which is a result of the shaping phase, placed into its containing layer.
struct PlacedDecoration {
    /// Decoration type - underline, double underline or strikethrough.
    enum class Type {
        NONE = 0,
        UNDERLINE,
        DOUBLE_UNDERLINE,
        STRIKE_THROUGH,
    } type;
    /// Start and end positions.
    Vector2f start, end;
    /// Thickness. In case of a double underline, this is the thickness of each of the lines.
    float thickness;
    /// Decoration color (incl. alpha).
    uint32_t color;
};
using PlacedDecorationPtr = std::unique_ptr<PlacedDecoration>;
using PlacedDecorations = std::vector<PlacedDecorationPtr>;

} // namespace odtr
