
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// TODO: Matus: Remove this include and unify
#include "textify-api.h"

#include "PlacedGlyph.h"
#include "PlacedDecoration.h"


namespace octopus {
    struct Text;
}

namespace textify {

struct Context;
struct TextShape;
typedef Context* ContextHandle;
typedef TextShape* TextShapeHandle;

struct ShapeTextResult_NEW {
    PlacedGlyphs placedGlyphs;
    PlacedDecorations placedDecorations;

    // TODO: Matus: Remove and replace -> this should be just PlacedGlyphs
    FRectangle textBounds;
};


/// Shape text.
TextShapeHandle shapeText_NEW(ContextHandle ctx,
                              const octopus::Text& text);

/// Draw text.
DrawTextResult drawText_NEW(ContextHandle ctx,
                            TextShapeHandle textShape,
                            void* pixels, int width, int height,
                            const DrawOptions& drawOptions = {});
}
