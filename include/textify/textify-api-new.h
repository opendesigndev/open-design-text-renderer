
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// TODO: Matus: Remove this include and unify
#include "textify-api.h"

namespace octopus {
    struct Text;
}

namespace textify {

struct Context;
struct TextShape;
typedef Context* ContextHandle;
typedef TextShape* TextShapeHandle;

/// Shape text.
TextShapeHandle shapePlacedText(ContextHandle ctx,
                                const octopus::Text& text);

/// Draw text.
// TODO: Matus: Make this function accept PlacedTextData instead of TextShapeHandle
DrawTextResult drawPlacedText(ContextHandle ctx,
                              TextShapeHandle textShape,
                              void* pixels, int width, int height,
                              const DrawOptions& drawOptions = {});
}
