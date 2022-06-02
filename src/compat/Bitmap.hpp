
#pragma once

#include <cstdlib>
#include <cstring>
#include <cstdint>
#include "basic-types.h"

#include <memory>

namespace textify {
namespace compat {

/// Reperesents a bitmap of 32-bit RGBA pixels in memory
template <typename T>
class Bitmap
{

public:
    enum Wrap { WRAP, WRAP_NO_OWN };

    typedef T PixelType;

    Bitmap();
    Bitmap(int width, int height);
    Bitmap(const T* pixels, int width, int height);
    Bitmap(Wrap, T* pixels, int width, int height);
    Bitmap(const Bitmap<T>& orig);
    Bitmap(Bitmap<T>&& orig);
    ~Bitmap();
    Bitmap<T>& operator=(const Bitmap<T>& orig);
    Bitmap<T>& operator=(Bitmap<T>&& orig);
    void clear();
    void fill(T value);

    inline const T* pixels() const { return pixels_; }
    inline T* pixels() { return pixels_; }
    inline int width() const { return width_; }
    inline int height() const { return height_; }
    inline Vector2i dimensions() const { return Vector2i { width_, height_ }; }
    PixelFormat format() const;

    T& operator()(int x, int y);
    T operator()(int x, int y) const;

private:
    T* pixels_;
    int width_, height_;
    bool ownsData_ = true;     // MODIFIED for use in textify 2!!!
};

typedef Bitmap<Pixel8> BitmapGrayscale;
typedef Bitmap<Pixel32> BitmapRGBA;

template <typename T>
using BitmapPtr = std::shared_ptr<Bitmap<T> >;

typedef std::shared_ptr<BitmapGrayscale> BitmapGrayscalePtr;
typedef std::shared_ptr<BitmapRGBA> BitmapRGBAPtr;

template <typename T>
BitmapPtr<T> createBitmap(int width, int height);
template <typename T>
BitmapPtr<T> createBitmap(const T* pixels, int width, int height);
template <typename T>
BitmapPtr<T> duplicateBitmap(const Bitmap<T>& orig);

template <typename T>
Bitmap<T>::Bitmap() {
    width_ = 0, height_ = 0;
    pixels_ = nullptr;
}

template <typename T>
Bitmap<T>::Bitmap(int width, int height) : width_(width), height_(height) {
    #ifndef __EMSCRIPTEN__
        pixels_ = reinterpret_cast<T*>(malloc(sizeof(T)*width*height));
    #else
        if (!(pixels_ = reinterpret_cast<T*>(malloc(sizeof(T)*width*height)))) {
            width_ = 0, height_ = 0;
            return;
        }
    #endif
}

template <typename T>
Bitmap<T>::Bitmap(const T* pixels, int width, int height) : width_(width), height_(height) {
    #ifndef __EMSCRIPTEN__
        pixels_ = reinterpret_cast<T*>(malloc(sizeof(T)*width*height));
    #else
        if (!(pixels_ = reinterpret_cast<T*>(malloc(sizeof(T)*width*height)))) {
            width_ = 0, height_ = 0;
            return;
        }
    #endif
    memcpy(pixels_, pixels, sizeof(T)*width*height);
}

template <typename T>
Bitmap<T>::Bitmap(Wrap w, T* pixels, int width, int height) : pixels_(pixels), width_(width), height_(height), ownsData_(w != WRAP_NO_OWN) { }

template <typename T>
Bitmap<T>::Bitmap(const Bitmap<T>& orig) : width_(orig.width_), height_(orig.height_) {
    #ifndef __EMSCRIPTEN__
        pixels_ = reinterpret_cast<T*>(malloc(sizeof(T)*width_*height_));
    #else
        if (!(pixels_ = reinterpret_cast<T*>(malloc(sizeof(T)*width_*height_)))) {
            width_ = 0, height_ = 0;
            return;
        }
    #endif
    memcpy(pixels_, orig.pixels_, sizeof(T)*width_*height_);
}

template <typename T>
Bitmap<T>::Bitmap(Bitmap<T>&& orig) : width_(orig.width_), height_(orig.height_), pixels_(orig.pixels_), ownsData_(orig.ownsData_) {
    orig.width_ = 0;
    orig.height_ = 0;
    orig.pixels_ = nullptr;
}

template <typename T>
Bitmap<T>::~Bitmap() {
    if (ownsData_) {
        free(pixels_);
    }
}

template <typename T>
Bitmap<T>& Bitmap<T>::operator=(const Bitmap<T>& orig) {
    if (this != &orig) {
        #ifndef __EMSCRIPTEN__
            if (ownsData_) {
                pixels_ = reinterpret_cast<T*>(realloc(pixels_, sizeof(T)*orig.width_*orig.height_));
            } else {
                pixels_ = reinterpret_cast<T*>(malloc(sizeof(T)*orig.width_*orig.height_));
            }
            ownsData_ = true;
        #else
            T* newPixels = nullptr;
            if (ownsData_) {
                newPixels = reinterpret_cast<T*>(realloc(pixels_, sizeof(T)*orig.width_*orig.height_));
            } else {
                newPixels = reinterpret_cast<T*>(malloc(sizeof(T)*orig.width*orig.height));
            }
            if (newPixels) {
                ownsData_ = true;
                pixels_ = newPixels;
            } else {
                if (ownsData_)
                    free(pixels_);
                pixels_ = nullptr;
                width_ = 0, height_ = 0;
                return *this;
            }
        #endif
        width_ = orig.width_, height_ = orig.height_;
        memcpy(pixels_, orig.pixels_, sizeof(T)*width_*height_);
    }
    return *this;
}

template <typename T>
Bitmap<T>& Bitmap<T>::operator=(Bitmap<T>&& orig) {
    if (this != &orig) {
        if (ownsData_)
            free(pixels_);
        width_ = orig.width_, height_ = orig.height_;
        pixels_ = orig.pixels_;
        ownsData_ = orig.ownsData_;
        orig.width_ = 0;
        orig.height_ = 0;
        orig.pixels_ = nullptr;
    }
    return *this;
}

template <typename T>
void Bitmap<T>::clear() {
    memset(pixels_, 0, sizeof(T)*width_*height_);
}

template <typename T>
void Bitmap<T>::fill(T value) {
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            pixels_[width_*y+x] = value;
        }
    }
}

template <typename T>
T& Bitmap<T>::operator()(int x, int y) {
    return pixels_[width_*y+x];
}

template <typename T>
T Bitmap<T>::operator()(int x, int y) const {
    return pixels_[width_*y+x];
}

template <typename T>
BitmapPtr<T> createBitmap(int width, int height) {
    if (T* pixels = reinterpret_cast<T*>(malloc(sizeof(T)*width*height))) {
        return std::make_shared<Bitmap<T> >(Bitmap<T>::WRAP, pixels, width, height);
    }
    // TODO log error
    return nullptr;
}

template <typename T>
BitmapPtr<T> createBitmap(const T* pixels, int width, int height) {
    if (T* duplicatePixels = reinterpret_cast<T*>(malloc(sizeof(T)*width*height))) {
        memcpy(duplicatePixels, pixels, sizeof(T)*width*height);
        return std::make_shared<Bitmap<T> >(Bitmap<T>::WRAP, duplicatePixels, width, height);
    }
    return nullptr;
}

template <typename T>
BitmapPtr<T> duplicateBitmap(const Bitmap<T>& orig) {
    return createBitmap(orig.pixels(), orig.width(), orig.height());
}

inline BitmapRGBAPtr createBitmapRGBA(int width, int height) { return createBitmap<Pixel32>(width, height); }
inline BitmapRGBAPtr createBitmapRGBA(const Vector2i &dimensions) { return createBitmap<Pixel32>(dimensions.x, dimensions.y); }

} // namespace compat
} // namespace textify
