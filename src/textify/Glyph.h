#pragma once

#include "base.h"
#include "compat/Bitmap.hpp"

namespace odtr {

class Face;

/// Base class for single glyph bitmaps
class Glyph
{
public:
    virtual ~Glyph() {}

    virtual void blit(Pixel32* dst, const IDims2& dDims, const compat::Vector2i& offset) const = 0;
    virtual int bitmapHeight() const = 0;
    virtual int bitmapWidth() const = 0;
    virtual void setColor(const Pixel32& c) = 0;
    virtual bool putBitmap(const FT_Bitmap &bitmap) = 0;
    virtual void scaleBitmap(float scale) = 0;

    void setDestination(const IPoint2& p);
    const IPoint2 &getDestination() const;

    void setOrigin(const FPoint2& p);
    const FPoint2 &getOrigin() const;

    compat::Rectangle getBitmapBounds() const;

    IVec2 bitmapBearing;          /// Position of bitmap relative to origin (and to baseline)
    spacing lsb_delta, rsb_delta; /// left / right side bearing delta, see FT_GlyphSlotRec_ for details
    FVec2 metricsBearing;         /// Position of the real glyph relative to origin (and to baseline) (unscaled)

protected:
    /// Position in the destination bitmap, computed using #bitmapBearing.
    IPoint2 destPos_;
    /// A real-valued ideal position of the glyph origin (on baseline) in the destination bitmap. Used for scaling to preserve the line.
    FPoint2 originPos_;
};
using GlyphPtr = std::unique_ptr<Glyph>;

//! Contains single grayscale glyph bitmap
class GrayGlyph : public Glyph
{
    friend class Face;

public:
    void blit(Pixel32* dst, const IDims2& dDims, const compat::Vector2i& offset) const override;
    int bitmapHeight() const override { return bitmap_->height(); }
    int bitmapWidth() const override { return bitmap_->width(); }
    void setColor(const Pixel32& color) override { color_ = color; }
    bool putBitmap(const FT_Bitmap &bitmap) override;
    void scaleBitmap(float scale) override;

private:
    compat::BitmapGrayscalePtr bitmap_;
    Pixel32 color_;
};

//! Contains single color glyph bitmap
class ColorGlyph : public Glyph
{
    friend class Face;

public:
    void blit(Pixel32* dst, const IDims2& dDims, const compat::Vector2i& offset) const override;
    int bitmapHeight() const override { return bitmap_->height(); }
    int bitmapWidth() const override { return bitmap_->width(); }
    //! Only set the color alpha
    void setColor(const Pixel32& color) override { alpha_ = color >> 24 & 0xff; }
    bool putBitmap(const FT_Bitmap &bitmap) override;
    void scaleBitmap(float scale) override;

private:
    compat::BitmapRGBAPtr bitmap_;
    Pixel32 alpha_;
};

} // namespace odtr
