
#include "bitmap-ops.hpp"

#include <algorithm>
#include "pixel-conversion.h"

namespace textify {
namespace compat {

void resampleBitmap(BitmapRGBA& dst, const BitmapRGBA& src) {
    int sw = src.width(), sh = src.height();
    int dw = dst.width(), dh = dst.height();
    double wr = double(sw)/double(dw), hr = double(sh)/double(dh);
    Pixel32 *pixel = dst.pixels();
    for (int dy = 0; dy < dh; ++dy) {
        double sy = wr*(double(dy)+.5)-.5;
        int isy = int(sy);
        float yw = float(sy)-float(isy);
        if (sy < 0.) {
            isy = 0;
            yw = 0.f;
        }
        if (isy > sh-2) {
            isy = sh-2;
            yw = 1.f;
        }
        for (int dx = 0; dx < dw; ++dx) {
            double sx = hr*(double(dx)+.5)-.5;
            int isx = int(sx);
            float xw = float(sx)-float(isx);
            if (sx < 0.) {
                isx = 0;
                xw = 0.f;
            }
            if (isx > sw-2) {
                isx = sw-2;
                xw = 1.f;
            }

            Color lt = pixelToColor(src(isx, isy));
            Color lb = pixelToColor(src(isx, isy+1));
            Color rt = pixelToColor(src(isx+1, isy));
            Color rb = pixelToColor(src(isx+1, isy+1));
            float ltw = (1.f-xw)*(1.f-yw);
            float lbw = (1.f-xw)*yw;
            float rtw = xw*(1.f-yw);
            float rbw = xw*yw;
            Color c = {
                ltw*lt.r + lbw*lb.r + rtw*rt.r + rbw*rb.r,
                ltw*lt.g + lbw*lb.g + rtw*rt.g + rbw*rb.g,
                ltw*lt.b + lbw*lb.b + rtw*rt.b + rbw*rb.b,
                ltw*lt.a + lbw*lb.a + rtw*rt.a + rbw*rb.a,
            };
            *pixel++ = colorToPixel(c);
        }
    }
}

void resampleBitmap(BitmapGrayscale& dst, const BitmapGrayscale& src) {
    int sw = src.width(), sh = src.height();
    int dw = dst.width(), dh = dst.height();
    double wr = double(sw)/double(dw), hr = double(sh)/double(dh);
    Pixel8 *pixel = dst.pixels();
    for (int dy = 0; dy < dh; ++dy) {
        double sy = wr*(double(dy)+.5)-.5;
        int isy = int(sy);
        float yw = float(sy)-float(isy);
        if (sy < 0.) {
            isy = 0;
            yw = 0.f;
        }
        if (isy > sh-2) {
            isy = sh-2;
            yw = 1.f;
        }
        for (int dx = 0; dx < dw; ++dx) {
            double sx = hr*(double(dx)+.5)-.5;
            int isx = int(sx);
            float xw = float(sx)-float(isx);
            if (sx < 0.) {
                isx = 0;
                xw = 0.f;
            }
            if (isx > sw-2) {
                isx = sw-2;
                xw = 1.f;
            }

            float lt = cComp_b2f(src(isx, isy));
            float lb = cComp_b2f(src(isx, isy+1));
            float rt = cComp_b2f(src(isx+1, isy));
            float rb = cComp_b2f(src(isx+1, isy+1));
            float ltw = (1.f-xw)*(1.f-yw);
            float lbw = (1.f-xw)*yw;
            float rtw = xw*(1.f-yw);
            float rbw = xw*yw;
            *pixel++ = cComp_f2b(ltw*lt + lbw*lb + rtw*rt + rbw*rb);
        }
    }
}

// TODO alpha correct weighing
void rescaleBitmap(BitmapRGBA& dst, const BitmapRGBA& src) {
    int sw = src.width(), sh = src.height();
    int dw = dst.width(), dh = dst.height();
    Vector2f invScale = { float(sw)/float(dw), float(sh)/float(dh) };
    Vector2f invUpscale = { std::min(invScale.x, 1.f), std::min(invScale.y, 1.f) };
    float invTotalWeight = 1.f/(invScale.x*invScale.y);

    Pixel32* pixel = dst.pixels();
    for (int y = 0; y < dh; ++y) {

        float t = float(y)*invScale.y;
        float b = float(y+1)*invScale.y;
        int it = int(t);
        int ib = std::min(int(b)+1, sh);

        for (int x = 0; x < dw; ++x) {

            float l = float(x)*invScale.x;
            float r = float(x+1)*invScale.x;
            int il = int(l);
            int ir = std::min(int(r)+1, sw);

            Color total = { };
            for (int py = it; py < ib; ++py) {
                float yw = std::min(std::min(b-float(py), float(py+1)-t), invUpscale.y);
                for (int px = il; px < ir; ++px) {
                    float w = std::min(std::min(r-float(px), float(px+1)-l), invUpscale.x)*yw;
                    assert(w >= 0.f);
                    Color sample = pixelToColor(src(px, py));
                    total.r += w*sample.r;
                    total.g += w*sample.g;
                    total.b += w*sample.b;
                    total.a += w*sample.a;
                }
            }
            total.r *= invTotalWeight;
            total.g *= invTotalWeight;
            total.b *= invTotalWeight;
            total.a *= invTotalWeight;
            *pixel++ = colorToPixel(total);
        }
    }
}

void rescaleBitmap(BitmapGrayscale& dst, const BitmapGrayscale& src) {
    int sw = src.width(), sh = src.height();
    int dw = dst.width(), dh = dst.height();
    Vector2f invScale = { float(sw)/float(dw), float(sh)/float(dh) };
    Vector2f invUpscale = { std::min(invScale.x, 1.f), std::min(invScale.y, 1.f) };
    float invTotalWeight = 1.f/(invScale.x*invScale.y);

    Pixel8* pixel = dst.pixels();
    for (int y = 0; y < dh; ++y) {

        float t = float(y)*invScale.y;
        float b = float(y+1)*invScale.y;
        int it = int(t);
        int ib = std::min(int(b)+1, sh);

        for (int x = 0; x < dw; ++x) {

            float l = float(x)*invScale.x;
            float r = float(x+1)*invScale.x;
            int il = int(l);
            int ir = std::min(int(r)+1, sw);

            float total = 0.f;
            for (int py = it; py < ib; ++py) {
                float yw = std::min(std::min(b-float(py), float(py+1)-t), invUpscale.y);
                for (int px = il; px < ir; ++px) {
                    float w = std::min(std::min(r-float(px), float(px+1)-l), invUpscale.x)*yw;
                    assert(w >= 0.f);
                    total += w*cComp_b2f(src(px, py));
                }
            }
            total *= invTotalWeight;
            *pixel++ = cComp_f2b(total);
        }
    }
}

// TODO alpha correct weighing
void rescaleBitmap(BitmapRGBA& dst, const BitmapRGBA& src, int mode, const Vector2f &align) {
    int sw = src.width(), sh = src.height();
    int dw = dst.width(), dh = dst.height();
    Vector2f invScale = { float(sw)/float(dw), float(sh)/float(dh) };
    Vector2f translate = { };
    if (mode != 0) {
        float scaleRatio = 1.f;
        float invSScale = 1.f;
        if (mode > 0) {
            scaleRatio = invScale.x/invScale.y;
            invSScale = std::max(invScale.x, invScale.y);
        }
        if (mode < 0) {
            scaleRatio = invScale.y/invScale.x;
            invSScale = std::min(invScale.x, invScale.y);
        }
        if (scaleRatio < 1.f)
            translate.x = -align.x*((invScale.y/invScale.x-1.f)*float(sw));
        if (scaleRatio > 1.f)
            translate.y = -align.y*((invScale.x/invScale.y-1.f)*float(sh));
        invScale = Vector2f { invSScale, invSScale };
    }

    Vector2f invUpscale = { std::min(invScale.x, 1.f), std::min(invScale.y, 1.f) };
    float invTotalWeight = 1.f/(invScale.x*invScale.y);

    Pixel32* pixel = dst.pixels();
    for (int y = 0; y < dh; ++y) {

        float t = float(y)*invScale.y+translate.y;
        float b = float(y+1)*invScale.y+translate.y;
        int it = std::max(int(t), 0);
        int ib = std::min(int(b+1.f), sh);

        for (int x = 0; x < dw; ++x) {

            float l = float(x)*invScale.x+translate.x;
            float r = float(x+1)*invScale.x+translate.x;
            int il = std::max(int(l), 0);
            int ir = std::min(int(r+1.f), sw);

            Color total = { };
            for (int py = it; py < ib; ++py) {
                float yw = std::min(std::min(b-float(py), float(py+1)-t), invUpscale.y);
                for (int px = il; px < ir; ++px) {
                    float w = std::min(std::min(r-float(px), float(px+1)-l), invUpscale.x)*yw;
                    assert(w >= 0.f);
                    Color sample = pixelToColor(src(px, py));
                    total.r += w*sample.r;
                    total.g += w*sample.g;
                    total.b += w*sample.b;
                    total.a += w*sample.a;
                }
            }
            total.r *= invTotalWeight;
            total.g *= invTotalWeight;
            total.b *= invTotalWeight;
            total.a *= invTotalWeight;
            *pixel++ = colorToPixel(total);
        }
    }
}

} // namespace compat
} // namespace textify
