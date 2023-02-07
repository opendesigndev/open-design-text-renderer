
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
    float glyphScale = 1.0f;
    const font_size desiredSize = face->isScalable() ? (placedGlyph.fontSize * scale) : placedGlyph.fontSize;
    const Result<font_size,bool> setSizeRes = face->setSize(desiredSize);

    if (setSizeRes && !face->isScalable()) {
        const float resizeFactor = desiredSize / setSizeRes.value();
        const float ascender = FreetypeHandle::from26_6fixed(face->getFtFace()->size->metrics.ascender) * resizeFactor;

        glyphScale = (ascender * scale) / setSizeRes.value();
    }

    const compat::Vector2f offset {
        static_cast<float>(placedGlyph.placement.topLeft.x - std::floor(placedGlyph.placement.topLeft.x)),
        static_cast<float>(placedGlyph.placement.topLeft.y - std::floor(placedGlyph.placement.topLeft.y)),
    };
    // If the face is scalable, scaling has already been applied in the setSize step.
    const ScaleParams glyphScaleParams = face->isScalable()
        ? ScaleParams { 1.0f, 1.0f }
        : ScaleParams { scale, glyphScale };

    GlyphPtr glyph = face->acquireGlyph(placedGlyph.glyphCodepoint, offset, glyphScaleParams, true, internalDisableHinting);
    if (!glyph) {
        return nullptr;
    }

    const Vector2f &placedGlyphPosition = placedGlyph.placement.topLeft;

    glyph->setDestination({
        static_cast<int>(std::floor(placedGlyphPosition.x * scale)),
        static_cast<int>(std::floor(placedGlyphPosition.y * scale))});
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

    const IPoint2 start {
        static_cast<int>(std::floor(pd.placement.topLeft.x * scale)),
        static_cast<int>(std::floor(pd.placement.topLeft.y * scale)) };
    const IPoint2 end {
        static_cast<int>(std::round(pd.placement.bottomRight.x * scale)),
        static_cast<int>(std::floor(pd.placement.bottomRight.y * scale)) };

    const float dt = (pd.placement.bottomLeft.y - pd.placement.topLeft.y) * scale;
    const int decorationThickness = (pd.type == PlacedDecoration::Type::DOUBLE_UNDERLINE)
        ? static_cast<int>(std::ceil(dt / 2.5f * (2.0f / 3.0f)))
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

void debug_drawRectangle(compat::BitmapRGBA &bitmap, const compat::FRectangle &rectangle) {
    const compat::Rectangle r = utils::outerRect(rectangle);

    const Pixel32 color = 0x55008888;

    BitmapWriter w = BitmapWriter(bitmap);

    for (int x = r.l; x < r.w; x += 1) {
        w.write(x, r.t, color);
        w.write(x, r.h - 1, color);
    }
    for (int y = r.t; y < r.h; y += 1) {
        w.write(r.l, y, color);
        w.write(r.w - 1, y, color);
    }
}

void debug_drawBitmapBoundaries(compat::BitmapRGBA &bitmap, int width, int height) {
    const Pixel32 color = 0x55008800;

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

void debug_drawBitmapGrid(compat::BitmapRGBA &bitmap, int width, int height) {
    const Pixel32 color = 0x11888888;

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

void debug_drawGlyphBoundingRectangle(compat::BitmapRGBA &bitmap, const PlacedGlyph &pg, RenderScale scale) {
    const Pixel32 bottomLeftColor = 0x55000088;
    const Pixel32 topRightColor = 0x55880000;
    BitmapWriter w = BitmapWriter(bitmap);

    const PlacedGlyph::QuadCorners &placement = pg.placement;

    const float width = (pg.placement.topRight.x - pg.placement.topLeft.x) * scale;
    const float height = (pg.placement.bottomLeft.y - pg.placement.topLeft.y) * scale;

    const Vector2f tl {
        pg.placement.topLeft.x * scale,
        pg.placement.topLeft.y * scale,
    };
    for (int x = tl.x; x < tl.x + width; x += 2) {
        w.write(x, tl.y + height, bottomLeftColor);
        w.write(x, tl.y, topRightColor);
    }
    for (int y = tl.y; y < tl.y + height; y += 2) {
        w.write(tl.x, y, bottomLeftColor);
        w.write(tl.x + width, y, topRightColor);
    }
}

} // namespace priv
} // namespace textify
