#include "Glyph.h"
#include "compat/bitmap-ops.hpp"
#include "compat/pixel-conversion.h"

#include <cmath>

namespace textify {

using namespace compat;

void Glyph::setDestination(const IPoint2& p) {
    destPos_ = p;
}
const IPoint2 &Glyph::getDestination() const {
    return destPos_;
}

void Glyph::setOrigin(const FPoint2& p) {
    originPos_ = p;
}
const FPoint2 &Glyph::getOrigin() const {
    return originPos_;
}

Rectangle Glyph::getBitmapBounds() const
{
    return {destPos_.x, destPos_.y, bitmapWidth(), bitmapHeight()};
}

static const unsigned char * getBitmapRow(const FT_Bitmap &bitmap, unsigned int row) {
    return bitmap.buffer+(bitmap.pitch >= 0 ? row*(unsigned int) bitmap.pitch : (bitmap.rows-row-1)*(unsigned int) -bitmap.pitch);
}

static void convertGrayscaleBitmap(Pixel8 *dst, const FT_Bitmap &bitmap) {
    const unsigned int rowLength = (unsigned int) abs(bitmap.pitch);
    switch (bitmap.pixel_mode) {
        case FT_PIXEL_MODE_MONO: // 1 bit per pixel
            for (unsigned int row = 0; row < bitmap.rows; ++row) {
                const unsigned char *src = getBitmapRow(bitmap, row);
                for (unsigned int x = 0, block = 0; block < rowLength; ++block, ++src) {
                    for (unsigned char mask = (unsigned char) 0x80u; mask && x < bitmap.width; mask >>= 1, ++x)
                        *dst++ = !!(*src&mask)*(unsigned char) 0xffu;
                }
            }
            break;
        case FT_PIXEL_MODE_GRAY: // 8 bits per pixel
            if (bitmap.num_grays == 256u) {
                if (bitmap.pitch == bitmap.width)
                    memcpy(dst, bitmap.buffer, sizeof(Pixel8)*bitmap.width*bitmap.rows);
                else for (unsigned int row = 0; row < bitmap.rows; ++row, dst += bitmap.width)
                    memcpy(dst, getBitmapRow(bitmap, row), sizeof(Pixel8)*bitmap.width);
            } else if (bitmap.num_grays >= 2u && bitmap.num_grays <= 256u) {
                for (unsigned int row = 0; row < bitmap.rows; ++row) {
                    const unsigned char *src = getBitmapRow(bitmap, row);
                    for (unsigned int x = 0; x < bitmap.width; ++x)
                        *dst++ = (unsigned char) ((unsigned) *src++*0xffu/(bitmap.num_grays-1));
                }
            } else {
                // Log::instance.log(Log::TEXTIFY, Log::ERROR, "Unexpected value of FT_Bitmap::num_grays!");
                memset(dst, 0, sizeof(Pixel8)*bitmap.width*bitmap.rows);
            }
            break;
        case FT_PIXEL_MODE_GRAY2: // 2 bits per pixel
            for (unsigned int row = 0; row < bitmap.rows; ++row) {
                const unsigned char *src = getBitmapRow(bitmap, row);
                for (unsigned int x = 0, block = 0; block < rowLength; ++block, ++src) {
                    for (int k = 6; k >= 0 && x < bitmap.width; k -= 2, ++x)
                        *dst++ = (*src>>k&0x03u)*(unsigned char) 0x55u; // 0x03 * 0x55 == 0xff
                }
            }
            break;
        case FT_PIXEL_MODE_GRAY4: // 4 bits per pixel
            for (unsigned int row = 0; row < bitmap.rows; ++row) {
                const unsigned char *src = getBitmapRow(bitmap, row);
                for (unsigned int x = 0, block = 0; block < rowLength; ++block, ++src) {
                    for (int k = 4; k >= 0 && x < bitmap.width; k -= 4, ++x)
                        *dst++ = (*src>>k&0x0fu)*(unsigned char) 0x11u; // 0x0f * 0x11 == 0xff
                }
            }
            break;
        default:
            // Log::instance.log(Log::TEXTIFY, Log::ERROR, "Unexpected glyph pixel format");
            memset(dst, 0, sizeof(Pixel8)*bitmap.width*bitmap.rows);
    }
}

bool GrayGlyph::putBitmap(const FT_Bitmap &bitmap) {
    if ((bitmap_ = createBitmap<Pixel8>(bitmap.width, bitmap.rows))) {
        convertGrayscaleBitmap(bitmap_->pixels(), bitmap);
        return true;
    }
    return false;
}

bool ColorGlyph::putBitmap(const FT_Bitmap &bitmap) {
    switch (bitmap.pixel_mode) {
        case FT_PIXEL_MODE_NONE:
            return false;
        case FT_PIXEL_MODE_MONO:
        case FT_PIXEL_MODE_GRAY:
        case FT_PIXEL_MODE_GRAY2:
        case FT_PIXEL_MODE_GRAY4:
            if ((bitmap_ = compat::createBitmapRGBA(bitmap.width, bitmap.rows))) {
                Pixel8 *intermediate = reinterpret_cast<Pixel8 *>(bitmap_->pixels());
                convertGrayscaleBitmap(intermediate, bitmap);
                Pixel32 *dst = bitmap_->pixels()+bitmap.width*bitmap.rows;
                const Pixel8 *src = intermediate+bitmap.width*bitmap.rows;
                while (--src >= intermediate) {
                    Pixel32 p = 0xff000000u;
                    p |= (Pixel32) *src;
                    p |= (Pixel32) *src<<8;
                    p |= (Pixel32) *src<<16;
                    *--dst = p;
                }
            }
            break;
        case FT_PIXEL_MODE_BGRA:
            if ((bitmap_ = compat::createBitmapRGBA(bitmap.width, bitmap.rows))) {
                Pixel32 *dst = bitmap_->pixels();
                for (unsigned int row = 0; row < bitmap.rows; ++row) {
                    const unsigned char *src = getBitmapRow(bitmap, row);
                    for (unsigned int x = 0; x < bitmap.width; ++x) {
                        Pixel32 p = 0u;
                        p |= (Pixel32) *src++<<16;
                        p |= (Pixel32) *src++<<8;
                        p |= (Pixel32) *src++;
                        p |= (Pixel32) *src++<<24;
                        *dst++ = p;
                    }
                }
            }
            break;
        case FT_PIXEL_MODE_LCD:
            if ((bitmap_ = compat::createBitmapRGBA(bitmap.width/3, bitmap.rows))) {
                Pixel32 *dst = bitmap_->pixels();
                for (unsigned int row = 0; row < bitmap.rows; ++row) {
                    const unsigned char *src = getBitmapRow(bitmap, row);
                    for (unsigned int x = 3; x <= bitmap.width; x += 3) {
                        Pixel32 p = 0xff000000u;
                        p |= (Pixel32) *src++;
                        p |= (Pixel32) *src++<<8;
                        p |= (Pixel32) *src++<<16;
                        *dst++ = p;
                    }
                }
            }
            break;
        case FT_PIXEL_MODE_LCD_V:
            if ((bitmap_ = compat::createBitmapRGBA(bitmap.width, bitmap.rows/3))) {
                Pixel32 *dst = bitmap_->pixels();
                for (unsigned int row = 3; row <= bitmap.rows; row += 3) {
                    const unsigned char *src[3] = {
                        getBitmapRow(bitmap, row-3),
                        getBitmapRow(bitmap, row-2),
                        getBitmapRow(bitmap, row-1),
                    };
                    for (unsigned int x = 0; x < bitmap.width; ++x) {
                        Pixel32 p = 0xff000000u;
                        p |= (Pixel32) *src[0]++;
                        p |= (Pixel32) *src[1]++<<8;
                        p |= (Pixel32) *src[2]++<<16;
                        *dst++ = p;
                    }
                }
            }
            break;
        default:
            // Log::instance.log(Log::TEXTIFY, Log::ERROR, "Unexpected glyph pixel format");
            ;
    }
    return true;
}

void GrayGlyph::blit(Pixel32* dst, const IDims2& dDims, const Vector2i& offset) const
{
    if (!bitmap_ || !bitmap_->pixels()) {
        // Log::instance.log(Log::TEXTIFY, Log::ERROR, "Trying to blit an empty bitmap.");
        return;
    }

    if (!dst) {
        // Log::instance.log(Log::TEXTIFY, Log::ERROR, "Trying to blit into an empty bitmap.");
        return;
    }

    Color premultipliedColor = pixelToColor(color_);
    premultipliedColor.r *= premultipliedColor.a;
    premultipliedColor.g *= premultipliedColor.a;
    premultipliedColor.b *= premultipliedColor.a;

    Pixel8 *src = bitmap_->pixels();

    for (int sy = 0; sy < bitmap_->height(); ++sy) {
        for (int sx = 0; sx < bitmap_->width(); ++sx, ++src) {
            const int dx = destPos_.x + sx + offset.x;
            const int dy = destPos_.y + sy + offset.y;

            if (dx < 0 || dy < 0 || dx >= dDims.x || dy >= dDims.y) {
                continue;
            }

            const float letterAlpha = *src / 255.f;
            Color srcColor = premultipliedColor;
            srcColor.r *= letterAlpha;
            srcColor.g *= letterAlpha;
            srcColor.b *= letterAlpha;
            srcColor.a *= letterAlpha;

            Color dstColor = pixelToColor(dst[dDims.x * dy + dx]);
            dstColor.r *= 1 - srcColor.a;
            dstColor.g *= 1 - srcColor.a;
            dstColor.b *= 1 - srcColor.a;
            dstColor.a *= 1 - srcColor.a;

            dst[dDims.x * dy + dx] = colorToPixel(srcColor + dstColor);
        }
    }
}

void ColorGlyph::blit(Pixel32* dst, const IDims2& dDims, const Vector2i& offset) const
{
    if (!bitmap_ || !bitmap_->pixels()) {
        // Log::instance.log(Log::TEXTIFY, Log::ERROR, "Trying to blit an empty bitmap.");
        return;
    }

    if (!dst) {
        // Log::instance.log(Log::TEXTIFY, Log::ERROR, "Trying to blit into an empty bitmap.");
        return;
    }

    Pixel32 *src = bitmap_->pixels();

    for (int sy = 0; sy < bitmap_->height(); ++sy) {
        for (int sx = 0; sx < bitmap_->width(); ++sx, ++src) {
            const int dx = destPos_.x + sx + offset.x;
            const int dy = destPos_.y + sy + offset.y;

            if (dx < 0 || dy < 0 || dx >= dDims.x || dy >= dDims.y) {
                continue;
            }

            // color glyphs are already alpha premultiplied
            const Color srcColor = pixelToColor(*src);

            Color dstColor = compat::pixelToColor(dst[dDims.x * dy + dx]);
            dstColor.r *= 1 - srcColor.a;
            dstColor.g *= 1 - srcColor.a;
            dstColor.b *= 1 - srcColor.a;
            dstColor.a *= 1 - srcColor.a;

            dst[dDims.x * dy + dx] = colorToPixel(srcColor + dstColor);
        }
    }
}

void GrayGlyph::scaleBitmap(float scale)
{
    int w = (int) ceilf(scale*(float) bitmap_->width());
    int h = (int) ceilf(scale*(float) bitmap_->height());
    if (w == bitmap_->width() && h == bitmap_->height())
        return;
    if (BitmapGrayscalePtr scaledBitmap = createBitmap<Pixel8>(w, h)) {
        resampleBitmap(*scaledBitmap, *bitmap_);
        bitmap_ = std::move(scaledBitmap);
    }
}

void ColorGlyph::scaleBitmap(float scale)
{
    int w = (int) ceilf(scale*(float) bitmap_->width());
    int h = (int) ceilf(scale*(float) bitmap_->height());
    if (w == bitmap_->width() && h == bitmap_->height())
        return;
    if (BitmapRGBAPtr scaledBitmap = createBitmapRGBA(w, h)) {
        resampleBitmap(*scaledBitmap, *bitmap_);
        bitmap_ = std::move(scaledBitmap);
    }
}

} // namespace textify
