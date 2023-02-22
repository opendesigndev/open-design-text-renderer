
#include "PlacedTextRendering.h"

#include <cmath>

#include <textify/PlacedGlyph.h>
#include <textify/PlacedDecoration.h>

#include "BitmapWriter.h"
#include "Glyph.h"
#include "Face.h"

#include "utils/utils.h"

namespace textify {
namespace priv {

GlyphPtr renderPlacedGlyph(const PlacedGlyph &placedGlyph,
                           const FacePtr &face,
                           RenderScale scale,
                           bool internalDisableHinting) {
    // TODO: apply placedGlyph rotation

    scale *= placedGlyph.scaling;

    float bitmapGlyphScale = 1.0f;
    const Result<font_size,bool> setSizeRes = face->setSize(placedGlyph.fontSize);

    if (setSizeRes && !face->isScalable()) {
        const float resizeFactor = placedGlyph.fontSize / setSizeRes.value();
        const float ascender = FreetypeHandle::from26_6fixed(face->getFtFace()->size->metrics.ascender) * resizeFactor;

        bitmapGlyphScale = (ascender * scale) / setSizeRes.value();
    }

    const Vector2f originOnBitmap {
        std::floor(placedGlyph.originPosition.x * scale),
        std::floor(placedGlyph.originPosition.y * scale),
    };
    const compat::Vector2f offset {
        placedGlyph.originPosition.x * scale - originOnBitmap.x,
        placedGlyph.originPosition.y * scale - originOnBitmap.y,
    };
    const ScaleParams glyphScaleParams { scale, bitmapGlyphScale };

    GlyphPtr glyph = face->acquireGlyph(placedGlyph.codepoint, offset, glyphScaleParams, true, internalDisableHinting);
    if (!glyph) {
        return nullptr;
    }

    glyph->setDestination({
        static_cast<int>(originOnBitmap.x + glyph->bitmapBearing.x),
        static_cast<int>(originOnBitmap.y - glyph->bitmapBearing.y) });

    glyph->setColor(placedGlyph.color);

    return glyph;
}

void drawGlyph(compat::BitmapRGBA &bitmap,
               const Glyph &glyph,
               const compat::Rectangle &viewArea) {
    const compat::Rectangle placedGlyphBounds = glyph.getBitmapBounds();

    // skips glyphs outside of view area
    if (!(placedGlyphBounds & viewArea)) {
        return;
    }

    const IDims2 destDims{ bitmap.width(), bitmap.height() };
    glyph.blit(bitmap.pixels(), destDims, compat::Vector2i{ 0, 0 });
}

void drawDecoration(compat::BitmapRGBA &bitmap,
                    const PlacedDecoration &pd,
                    RenderScale scale) {
    if (pd.type == PlacedDecoration::Type::NONE) {
        return;
    }

    const float dt = pd.thickness * (pd.type == PlacedDecoration::Type::DOUBLE_UNDERLINE ? 2.5f : 1.0f) * scale;
    const IPoint2 start {
        static_cast<int>(std::floor(pd.start.x * scale)),
        static_cast<int>(std::floor(pd.start.y * scale)) };
    const IPoint2 end {
        static_cast<int>(std::round(pd.end.x * scale)),
        static_cast<int>(std::floor(pd.end.y * scale)) };

    const int decorationThickness = (pd.type == PlacedDecoration::Type::DOUBLE_UNDERLINE)
        ? static_cast<int>(std::ceil(dt * (2.0f / (3.0f * 2.5f))))
        : static_cast<int>(std::ceil(dt));
    const int vOffset = static_cast<int>(decorationThickness * 0.5);

    BitmapWriter w = BitmapWriter(bitmap);

    for (int j = 0; j < end.x - start.x; j++) {
        const int penX = start.x + j;
        if (!w.checkH(penX)) {
            continue;
        }

        if (pd.type == PlacedDecoration::Type::DOUBLE_UNDERLINE) {
            for (int k = 0; k < decorationThickness; k++) {
                w.write(penX, start.y - vOffset - k, pd.color);
                w.write(penX, start.y + vOffset + k, pd.color);
            }
        } else /* UNDERLINE || STRIKETHROUGH */ {
            for (int k = 0; k < decorationThickness; k++) {
                w.write(penX, start.y - k, pd.color);
            }
        }
    }
}

void debug::drawPoint(compat::BitmapRGBA &bitmap, const compat::Vector2i &pos, int radius, Pixel32 color) {
    BitmapWriter w = BitmapWriter(bitmap);

    for (int x = pos.x-(radius-1) ; x <= pos.x+(radius-1); x += 1) {
        for (int y = pos.y-(radius-1); y <= pos.y+(radius-1); y += 1) {
            w.write(x, y, color);
        }
    }
}

void debug::drawHorizontalLine(compat::BitmapRGBA &bitmap, int y, Pixel32 color) {
    BitmapWriter w = BitmapWriter(bitmap);

    for (int x = 0 ; x < bitmap.width(); x += 1) {
        w.write(x, y, color);
    }
}

void debug::drawVerticalLine(compat::BitmapRGBA &bitmap, int x, Pixel32 color) {
    BitmapWriter w = BitmapWriter(bitmap);

    for (int y = 0 ; y < bitmap.height(); y += 1) {
        w.write(x, y, color);
    }
}

void debug::drawRectangle(compat::BitmapRGBA &bitmap, const compat::FRectangle &rectangle, Pixel32 color) {
    BitmapWriter w = BitmapWriter(bitmap);

    const compat::Rectangle r = utils::outerRect(rectangle);

    for (int x = r.l; x < r.w; x += 1) {
        w.write(x, r.t, color);
        w.write(x, r.h - 1, color);
    }
    for (int y = r.t; y < r.h; y += 1) {
        w.write(r.l, y, color);
        w.write(r.w - 1, y, color);
    }
}

void debug::drawBitmapBoundaries(compat::BitmapRGBA &bitmap, int width, int height, Pixel32 color) {
    BitmapWriter w = BitmapWriter(bitmap);

    for (int x = 0; x < width; x += 2) {
        w.write(x, 0, color);
        w.write(x, height - 1, color);
    }
    for (int y = 0; y < height; y += 2) {
        w.write(0, y, color);
        w.write(width - 1, y, color);
    }
}

void debug::drawBitmapGrid(compat::BitmapRGBA &bitmap, int width, int height, Pixel32 color) {
    BitmapWriter w = BitmapWriter(bitmap);

    for (int x = 10; x < width; x += 10) {
        for (int y = 0; y < height; y++) {
            w.write(x, y, color);
        }
    }
    for (int y = 10; y < height; y += 10) {
        for (int x = 0; x < width; x++) {
            w.write(x, y, color);
        }
    }
}

void debug::drawGlyphBoundingRectangle(compat::BitmapRGBA &bitmap, const FacePtr &face, const PlacedGlyph &pg, RenderScale scale) {
    scale *= pg.scaling;

    const Pixel32 bottomLeftColor = 0x55000088;
    const Pixel32 topRightColor = 0x55880000;
    BitmapWriter w = BitmapWriter(bitmap);

    float bitmapGlyphScale = 1.0f;
    const Result<font_size,bool> setSizeRes = face->setSize(pg.fontSize);

    if (setSizeRes && !face->isScalable()) {
        const float resizeFactor = pg.fontSize / setSizeRes.value();
        const float ascender = FreetypeHandle::from26_6fixed(face->getFtFace()->size->metrics.ascender) * resizeFactor;

        bitmapGlyphScale = (ascender * scale) / setSizeRes.value();
    }

    const Vector2f originOnBitmap {
        std::floor(pg.originPosition.x * scale),
        std::floor(pg.originPosition.y * scale),
    };
    const compat::Vector2f offset {
        pg.originPosition.x * scale - originOnBitmap.x,
        pg.originPosition.y * scale - originOnBitmap.y,
    };
    const ScaleParams glyphScaleParams { scale, bitmapGlyphScale };

    GlyphPtr glyph = face->acquireGlyph(pg.codepoint, offset, glyphScaleParams, true, false);
    if (!glyph) {
        return;
    }

    const IVec2 tl {
        static_cast<int>(originOnBitmap.x + glyph->bitmapBearing.x),
        static_cast<int>(originOnBitmap.y - glyph->bitmapBearing.y)
    };

    for (int x = tl.x; x < tl.x + glyph->bitmapWidth(); x += 2) {
        w.write(x, tl.y + glyph->bitmapHeight(), bottomLeftColor);
        w.write(x, tl.y, topRightColor);
    }
    for (int y = tl.y; y < tl.y + glyph->bitmapHeight(); y += 2) {
        w.write(tl.x, y, bottomLeftColor);
        w.write(tl.x + glyph->bitmapWidth(), y, topRightColor);
    }
}

} // namespace priv
} // namespace textify
