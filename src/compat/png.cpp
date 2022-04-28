
#include "png.h"

#ifndef __EMSCRIPTEN__

#include <png.h>
#include <vector>

namespace textify {
namespace compat {

static void pngError(png_structp png, png_const_charp message) {
    // Log::instance.log(Log::CORE_UTILS, Log::ERROR, std::string("Libpng error: ")+message);
}

static void pngWarning(png_structp png, png_const_charp message) {
    // Log::instance.log(Log::CORE_UTILS, Log::WARNING, std::string("Libpng warning: ")+message);
}

bool detectPngFormat(const byte* data, size_t len) {
    return len >= 4 && data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G';
}

BitmapRGBAPtr loadPngImage(FILE *file) {
    if (!file)
        return nullptr;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, pngError, pngWarning);
    if (!png)
        return nullptr;
    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, NULL, NULL);
        return nullptr;
    }
    // Error handling (archaic longjump model)
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, NULL); // Also destroys info
        return nullptr;
    }

    png_init_io(png, file);

    // (Here would be png_set_sig_bytes(pngPtr, 8); if we had consumed the signature already)
    // Read header and aggregate information
    png_read_info(png, info);
    png_uint_32 width = png_get_image_width(png, info);
    png_uint_32 height = png_get_image_height(png, info);
    png_uint_32 colorType = png_get_color_type(png, info);
    png_uint_32 transparency = png_get_valid(png, info, PNG_INFO_tRNS);

    // Make it so that indexed colors and less than 8 bits per channel are expanded into proper RGB/grayscale
    png_set_expand(png);
    // Make it so that 16bit channels are converted to regular 8bit ones
    png_set_strip_16(png);
    // Convert between grayscale and RGB
    if (!(colorType&PNG_COLOR_MASK_COLOR))
        png_set_gray_to_rgb(png);
    // Convert transparency
    if (transparency)
        png_set_tRNS_to_alpha(png);
    else if (!(colorType&PNG_COLOR_MASK_ALPHA))
        png_set_filler(png, 0xff, PNG_FILLER_AFTER);

    BitmapRGBAPtr bitmap = createBitmapRGBA(width, height);
    if (!bitmap) {
        png_destroy_read_struct(&png, &info, NULL);
        return nullptr;
    }

    // Create pointers to individual rows of the bitmap
    std::vector<png_bytep> rows;
    rows.resize(height);
    for (unsigned int i = 0; i < height; ++i)
        rows[i] = reinterpret_cast<png_bytep>(bitmap->pixels()+i*width);
    // Read the pixels
    png_read_image(png, &rows[0]);

    // Clean up
    png_destroy_read_struct(&png, &info, NULL);

    return bitmap;
}

bool savePngImage(const BitmapRGBA& bitmap, const char* filename) {
    if (!(bitmap.width() > 0 && bitmap.height() > 0))
        return false;
    FILE* file = fopen(filename, "wb");
    if (!file)
        return false;
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, pngError, pngWarning);
    if (!png) {
        fclose(file);
        return false;
    }
    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, NULL);
        fclose(file);
        return false;
    }
    // Error handling (archaic longjump model)
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info); // Also destroys info
        fclose(file);
        return false;
    }
    png_init_io(png, file);

    std::vector<const png_byte*> rows;
    rows.resize(bitmap.height());
    for (size_t i = 0; i < bitmap.height(); ++i)
        rows[i] = reinterpret_cast<const png_byte*>(bitmap.pixels()+i*bitmap.width());
    png_set_IHDR(png, info, bitmap.width(), bitmap.height(), 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_rows(png, info, const_cast<png_bytepp>(&rows[0]));
    png_write_png(png, info, PNG_TRANSFORM_IDENTITY, NULL);

    png_destroy_write_struct(&png, &info);
    fclose(file);
    return true;
}

} // namespace compat
} // namespace textify

#endif
