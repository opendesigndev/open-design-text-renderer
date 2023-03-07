
#pragma once

#include <cstring>
#include <cassert>
#include "basic-types.h"
#include "Bitmap.hpp"

namespace odtr {
namespace compat {

namespace {
void cropBoundsPair(Rectangle& subject, Rectangle& dependent, const Rectangle& cropBounds) {
    if (subject.l < cropBounds.l) {
        int shift = cropBounds.l-subject.l;
        subject.l += shift;
        subject.w -= shift;
        dependent.l += shift;
        dependent.w -= shift;
    }
    if (subject.t < cropBounds.t) {
        int shift = cropBounds.t-subject.t;
        subject.t += shift;
        subject.h -= shift;
        dependent.t += shift;
        dependent.h -= shift;
    }
    if (subject.l+subject.w > cropBounds.l+cropBounds.w) {
        int shift = (subject.l+subject.w)-(cropBounds.l+cropBounds.w);
        subject.w -= shift;
        dependent.w -= shift;
    }
    if (subject.t+subject.h > cropBounds.t+cropBounds.h) {
        int shift = (subject.t+subject.h)-(cropBounds.t+cropBounds.h);
        subject.h -= shift;
        dependent.h -= shift;
    }
}
}

template <typename T>
void blitBitmapSection(Bitmap<T>& dst, const Bitmap<T>& src, Rectangle dstArea, Rectangle srcArea) {
    cropBoundsPair(srcArea, dstArea, Rectangle { 0, 0, src.width(), src.height() });
    cropBoundsPair(dstArea, srcArea, Rectangle { 0, 0, dst.width(), dst.height() });
    assert(srcArea.w == dstArea.w && srcArea.h == dstArea.h);
    if (dstArea.w > 0 && dstArea.h > 0) {
        for (int row = 0; row < dstArea.h; ++row) {
            memcpy(&dst(dstArea.l, dstArea.t+row), src.pixels()+(srcArea.t+row)*src.width()+srcArea.l, sizeof(T)*dstArea.w);
        }
    }
}

template <typename T>
void blitBitmap(Bitmap<T>& dst, const Bitmap<T>& src, const Vector2i &dstPos) {
    blitBitmapSection(dst, src, Rectangle { dstPos.x, dstPos.y, src.width(), src.height() }, Rectangle { 0, 0, src.width(), src.height() });
}

/// Resamples bitmap using linear interpolation (good for upscaling)
void resampleBitmap(BitmapRGBA& dst, const BitmapRGBA& src);
void resampleBitmap(BitmapGrayscale& dst, const BitmapGrayscale& src);

/// Resamples bitmap by averaging all of the source pixels covered by each destination pixels
void rescaleBitmap(BitmapRGBA& dst, const BitmapRGBA& src);
void rescaleBitmap(BitmapGrayscale& dst, const BitmapGrayscale& src);
void rescaleBitmap(BitmapRGBA& dst, const BitmapRGBA& src, int mode, const Vector2f &align);

} // namespace compat
} // namespace odtr
