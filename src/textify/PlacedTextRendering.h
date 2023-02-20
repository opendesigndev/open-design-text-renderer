
#pragma once

#include "compat/Bitmap.hpp"
#include "text-format.h"

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

void debug_drawHorizontalLine(compat::BitmapRGBA &bitmap, int y, Pixel32 color = 0x88CC8888);
void debug_drawVerticalLine(compat::BitmapRGBA &bitmap, int x, Pixel32 color = 0x88CC8888);
void debug_drawRectangle(compat::BitmapRGBA &bitmap, const compat::FRectangle &rectangle, Pixel32 color = 0x55008888);
void debug_drawBitmapBoundaries(compat::BitmapRGBA &bitmap, int width, int height, Pixel32 color = 0x55008800);
void debug_drawBitmapGrid(compat::BitmapRGBA &bitmap, int width, int height, Pixel32 color = 0x11888888);
void debug_drawGlyphBoundingRectangle(compat::BitmapRGBA &bitmap, const PlacedGlyph &pg, RenderScale scale);

} // namespace priv
} // namespace textify
