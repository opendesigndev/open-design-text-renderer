
#include "PlacedTextRendering.h"

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
        static_cast<float>(placedGlyph.quadCorners.topLeft.x - floor(placedGlyph.quadCorners.topLeft.x)),
        static_cast<float>(placedGlyph.quadCorners.topLeft.y - floor(placedGlyph.quadCorners.topLeft.y)),
    };
    const ScaleParams glyphScaleParams { scale, glyphScale };

    GlyphPtr glyph = face->acquireGlyph(placedGlyph.glyphCodepoint, offset, glyphScaleParams, true, internalDisableHinting);
    if (!glyph) {
        return nullptr;
    }

    const Vector2f &placedGlyphPosition = placedGlyph.quadCorners.topLeft;

    glyph->setDestination({static_cast<int>(floor(placedGlyphPosition.x)), static_cast<int>(floor(placedGlyphPosition.y))});
    glyph->setColor(placedGlyph.color);

    return glyph;
}

void drawGlyph(compat::BitmapRGBA &bitmap,
               const Glyph &glyph,
               const compat::Rectangle &viewArea)
{
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
                    RenderScale scale,
                    const FacePtr &face) {
    if (pd.type == PlacedDecoration::Type::NONE) {
        return;
    }

    const IPoint2 start { static_cast<int>(floor(pd.xRange.first)), static_cast<int>(floor(pd.yOffset)) };
    const IPoint2 end { static_cast<int>(round(pd.xRange.last)), static_cast<int>(floor(pd.yOffset)) };

    const float decorationThickness = pd.thickness * scale;

    BitmapWriter w = BitmapWriter(bitmap);

    for (int j = 0; j < end.x - start.x; j++) {
        const int penX = start.x + j;
        if (!w.checkH(penX)) {
            continue;
        }

        if (pd.type == PlacedDecoration::Type::DOUBLE_UNDERLINE) {
            const int thickness = static_cast<int>(ceil(decorationThickness * 2.0 / 3.0));
            const int vOffset = static_cast<int>(thickness * 0.5);

            for (int k = 0; k < thickness; k++) {
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

void debug_drawGlyphBoundingRectangle(compat::BitmapRGBA &bitmap,
                                      const PlacedGlyph &pg)
{
    const Pixel32 bottomLeftColor = 0x55000088;
    const Pixel32 topRightColor = 0x55880000;
    BitmapWriter w = BitmapWriter(bitmap);

    for (int x = pg.quadCorners.bottomLeft.x; x < pg.quadCorners.bottomRight.x; x += 2) {
        w.write(x, pg.quadCorners.bottomLeft.y, bottomLeftColor);
        w.write(x, pg.quadCorners.topRight.y, topRightColor);
    }
    for (int y = pg.quadCorners.topLeft.y; y < pg.quadCorners.bottomLeft.y; y += 2) {
        w.write(pg.quadCorners.topLeft.x, y, bottomLeftColor);
        w.write(pg.quadCorners.bottomRight.x, y, topRightColor);
    }
}

} // namespace priv
} // namespace textify
