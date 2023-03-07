#pragma once

#include <cstdint>

namespace odtr {
namespace compat {

typedef unsigned char byte;
typedef uint32_t qchar;
/// A 8-bit grayscale pixel
typedef std::uint8_t Pixel8;
/// A 32-bit RGBA pixel, with LE alignment (0xAABBGGRR)
typedef std::uint32_t Pixel32;

/// A 2D vector / point / dimensions
template <typename T>
struct Vector2
{
    T x, y;

    bool operator==(const Vector2<T>& other) const;
    bool operator!=(const Vector2<T>& other) const;
    Vector2<T> operator+(const Vector2<T>& other) const;
    Vector2<T> operator-(const Vector2<T>& other) const;
    Vector2<T> operator*(const Vector2<T>& other) const;
    Vector2<T> operator*(T a) const;
    Vector2<T> operator/(T a) const;
    T length() const;
    template <typename S> inline explicit operator Vector2<S>() const { return Vector2<S> { (S) x, (S) y }; }
    template <typename S> friend Vector2<S> operator*(S a, const Vector2<S>& v);
    template <typename S> friend S dotProduct(const Vector2<S>& a, const Vector2<S>& b);
};

typedef Vector2<int> Vector2i;
typedef Vector2<float> Vector2f;

/// A float 3D vector / point / dimensions
struct Vector3f
{
    float x, y, z;

    float length() const;
};

/// A float 2x2 matrix (elements accessed as [column][row])
struct Matrix2f
{
    static const Matrix2f identity;

    float m[2][2];

    Matrix2f operator*(const Matrix2f& other) const;
    Vector2f operator*(const Vector2f& other) const;
    const float* operator[](int idx) const { return m[idx]; };
    float* operator[](int idx) { return m[idx]; };
    bool operator==(const Matrix2f& other) const;
    bool operator!=(const Matrix2f& other) const;
};
/// Throws std::invalid_argument if matrix determinant equals 0
Matrix2f inverse(const Matrix2f& m);

/// A float 3x3 matrix (elements accessed as [column][row])
struct Matrix3f
{
    static const Matrix3f identity;

    float m[3][3];

    Matrix3f operator*(const Matrix3f& other) const;
    Vector3f operator*(const Vector3f& other) const;
    Matrix3f & operator*=(const Matrix3f& other);
    const float* operator[](int idx) const { return m[idx]; };
    float* operator[](int idx) { return m[idx]; };
    bool operator==(const Matrix3f& other) const;
    bool operator!=(const Matrix3f& other) const;
};

/// Throws std::invalid_argument if matrix determinant equals 0
Matrix3f inverse(const Matrix3f& m);

/// A rectangle representing various bounds (AABB)
template <typename T>
struct TRectangle
{
    // Returns true if the specified point lies within the rectangle
    bool contains(T x, T y) const;
    // Coordinate translation (other relative to this / this relative to other)
    TRectangle<T> operator+(const TRectangle<T>& other) const;
    TRectangle<T> operator-(const TRectangle<T>& other) const;
    // Intersection of two rectangles
    TRectangle<T> operator&(const TRectangle<T>& other) const;
    // Bounding box of two rectangles
    TRectangle<T> operator|(const TRectangle<T>& other) const;
    // Rectangle translation
    TRectangle<T> operator+(const Vector2<T>& v) const;
    TRectangle<T> operator-(const Vector2<T>& v) const;

    bool operator==(const TRectangle<T>& other) const;
    bool operator!=(const TRectangle<T>& other) const;

    TRectangle<T>& operator+=(const TRectangle<T>& other);
    TRectangle<T>& operator-=(const TRectangle<T>& other);
    TRectangle<T>& operator&=(const TRectangle<T>& other);
    TRectangle<T>& operator|=(const TRectangle<T>& other);
    TRectangle<T>& operator+=(const Vector2<T>& v);
    TRectangle<T>& operator-=(const Vector2<T>& v);

    explicit operator bool() const;
    TRectangle<T> canonical() const;
    Vector2<T> dimensions() const;

    T l, t;
    T w, h;
};

typedef TRectangle<int> Rectangle;
typedef TRectangle<float> FRectangle;

extern const Rectangle UNSPECIFIED_BOUNDS;
extern const Rectangle INFINITE_BOUNDS;

Rectangle translateRect(const Rectangle& r, const Vector2f& offset);

/// A floating-point color value (range 0 .. 1)
struct Color
{
    float r, g, b, a;

    static const Color TRANSPARENT;

    static Color makeRGB(float r, float g, float b) { return {r, g, b, 1.0f}; }
    static Color makeRGBA(float r, float g, float b, float a) { return {r, g, b, a}; }
    static Color makePremultiplied(const Color& straight) { return makePremultiplied(straight.r, straight.g, straight.b, straight.a); }
    static Color makePremultiplied(float r, float g, float b, float a) { return {r * a, g * a, b * a, a}; }

    bool operator==(const Color& that) const { return r == that.r && g == that.g && b == that.b && a == that.a; }
    bool operator!=(const Color& that) const { return r != that.r || g != that.g || b != that.b || a != that.a; }
    Color& operator+=(const Color& that);
    Color operator+(const Color& that) const;
};

enum class PixelFormat
{
    INVALID,
    GRAYSCALE_BYTE,
    GRAYSCALE_FLOAT,
    RGB_BYTE,
    RGB_FLOAT,
    RGBA_BYTE,
    RGBA_FLOAT,
    ALPHA_BYTE,
    ALPHA_FLOAT
};

// ... continues ...

} // namespace compat
} // namespace odtr
