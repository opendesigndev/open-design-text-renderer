
#pragma once

#include "compat/Bitmap.hpp"

namespace odtr {
namespace priv {

class BitmapWriter
{
public:
    explicit BitmapWriter(compat::BitmapRGBA &bitmap);

    bool checkV(int y);
    bool checkH(int x);

    void write(int x, int y, compat::Pixel32 color);

private:
    compat::BitmapRGBA &bitmap_;
};

} // namespace priv
} // namespace odtr
