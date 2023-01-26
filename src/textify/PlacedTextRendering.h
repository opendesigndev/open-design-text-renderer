
#pragma once

#include "compat/Bitmap.hpp"

// Forward declarations
namespace textify {
struct PlacedGlyph;
struct PlacedDecoration;
class Glyph;
class FacePtr;
using GlyphPtr = std::unique_ptr<Glyph>;
typedef float RenderScale;
} // namespace textify

namespace textify {
namespace priv {

GlyphPtr renderPlacedGlyph(const PlacedGlyph &placedGlyph,
                           const FacePtr &face,
                           RenderScale scale,
                           bool internalDisableHinting);

void drawGlyph(compat::BitmapRGBA &bitmap,
               const Glyph &glyph,
               const compat::Rectangle &viewArea);

void drawDecoration(compat::BitmapRGBA &bitmap,
                    const PlacedDecoration &pd,
                    RenderScale scale);

void debug_drawRectangle(compat::BitmapRGBA &bitmap, const compat::FRectangle &rectangle);
void debug_drawBitmapBoundaries(compat::BitmapRGBA &bitmap, int width, int height);
void debug_drawBitmapGrid(compat::BitmapRGBA &bitmap, int width, int height);
void debug_drawGlyphBoundingRectangle(compat::BitmapRGBA &bitmap, const PlacedGlyph &pg);

} // namespace priv
} // namespace textify
