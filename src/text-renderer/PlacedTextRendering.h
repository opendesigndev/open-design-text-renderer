
#pragma once

#include "../compat/Bitmap.hpp"
#include "text-format.h"

// Forward declarations
namespace odtr {
struct PlacedGlyph;
struct PlacedDecoration;
class Glyph;
class FacePtr;
using GlyphPtr = std::unique_ptr<Glyph>;
typedef float RenderScale;
} // namespace odtr

namespace odtr {
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

namespace debug {
void drawPoint(compat::BitmapRGBA &bitmap, const compat::Vector2i &pos, int radius = 1, Pixel32 color = 0xFF2222FF);
void drawHorizontalLine(compat::BitmapRGBA &bitmap, int y, Pixel32 color = 0x88CC8888);
void drawVerticalLine(compat::BitmapRGBA &bitmap, int x, Pixel32 color = 0x88CC8888);
void drawRectangle(compat::BitmapRGBA &bitmap, const compat::FRectangle &rectangle, Pixel32 color = 0x55008888);
void drawBitmapBoundaries(compat::BitmapRGBA &bitmap, int width, int height, Pixel32 color = 0x55008800);
void drawBitmapGrid(compat::BitmapRGBA &bitmap, int width, int height, Pixel32 color = 0x11888888);
void drawGlyphBoundingRectangle(compat::BitmapRGBA &bitmap, const FacePtr &face, const PlacedGlyph &pg, RenderScale scale);
}

} // namespace priv
} // namespace odtr
