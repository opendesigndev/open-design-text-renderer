#include "basic-types.h"

#include <cmath>
#include <algorithm>
#include "arithmetics.hpp"

namespace textify {
namespace compat {

template <typename T>
bool Vector2<T>::operator==(const Vector2<T>& other) const {
    return x == other.x && y == other.y;
}

template <typename T>
bool Vector2<T>::operator!=(const Vector2<T>& other) const {
    return !operator==(other);
}

template <typename T>
Vector2<T> Vector2<T>::operator+(const Vector2<T>& other) const {
    return Vector2<T> { x+other.x, y+other.y };
}

template <typename T>
Vector2<T> Vector2<T>::operator-(const Vector2<T>& other) const {
    return Vector2<T> { x-other.x, y-other.y };
}

template <typename T>
Vector2<T> Vector2<T>::operator*(const Vector2<T>& other) const {
    return Vector2<T> { x*other.x, y*other.y };
}

template <typename T>
Vector2<T> Vector2<T>::operator*(T a) const {
    return Vector2<T> { x*a, y*a };
}

template <typename T>
Vector2<T> Vector2<T>::operator/(T a) const {
    return Vector2<T> { x/a, y/a };
}

template <typename T>
T Vector2<T>::length() const {
    return (T) sqrt(x*x+y*y);
}

template <typename T>
Vector2<T> operator*(T a, const Vector2<T>& v) {
    return Vector2<T> { a*v.x, a*v.y };
}

template <typename T>
T dotProduct(const Vector2<T>& a, const Vector2<T>& b) {
    return a.x*b.x+a.y*b.y;
}

template struct Vector2<int>;
template struct Vector2<float>;
template Vector2<int> operator*<int>(int a, const Vector2<int>& v);
template Vector2<float> operator*<float>(float a, const Vector2<float>& v);
template int dotProduct<int>(const Vector2<int>& a, const Vector2<int>& b);
template float dotProduct<float>(const Vector2<float>& a, const Vector2<float>& b);

float Vector3f::length() const {
    return sqrtf(x*x + y*y + z*z);
}

const Matrix2f Matrix2f::identity = { { { 1.f, 0.f }, { 0.f, 1.f } } };

Matrix2f Matrix2f::operator * (const Matrix2f& other) const {
    Matrix2f result;

    result[0][0] = m[0][0] * other[0][0] + m[1][0] * other[0][1];
    result[0][1] = m[0][1] * other[0][0] + m[1][1] * other[0][1];
    result[1][0] = m[0][0] * other[1][0] + m[1][0] * other[1][1];
    result[1][1] = m[0][1] * other[1][0] + m[1][1] * other[1][1];

    return result;
}

Vector2f Matrix2f::operator * (const Vector2f& other) const {
    Vector2f result;

    result.x = m[0][0] * other.x + m[1][0] * other.y;
    result.y = m[0][1] * other.x + m[1][1] * other.y;

    return result;
}

Matrix2f inverse(const Matrix2f& m) {
    float det = m[0][0] * m[1][1] - m[1][0] * m[0][1];
    if (det == 0.f)
        return Matrix2f { };

    float invDet = 1.0f / det;

    Matrix2f res;
    res[0][0] = invDet * m[1][1];
    res[0][1] = invDet * -m[0][1];
    res[1][0] = invDet * -m[1][0];
    res[1][1] = invDet * m[0][0];

    return res;
}

bool Matrix2f::operator==(const Matrix2f& other) const {
    return
        m[0][0] == other[0][0] &&
        m[0][1] == other[0][1] &&
        m[1][0] == other[1][0] &&
        m[1][1] == other[1][1];
}

bool Matrix2f::operator!=(const Matrix2f& other) const {
    return !operator==(other);
}

const Matrix3f Matrix3f::identity = { { { 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, 1.f } } };

Matrix3f Matrix3f::operator*(const Matrix3f& other) const {
    Matrix3f result;
    result[0][0] = m[0][0]*other[0][0] + m[1][0]*other[0][1] + m[2][0]*other[0][2];
    result[0][1] = m[0][1]*other[0][0] + m[1][1]*other[0][1] + m[2][1]*other[0][2];
    result[0][2] = m[0][2]*other[0][0] + m[1][2]*other[0][1] + m[2][2]*other[0][2];
    result[1][0] = m[0][0]*other[1][0] + m[1][0]*other[1][1] + m[2][0]*other[1][2];
    result[1][1] = m[0][1]*other[1][0] + m[1][1]*other[1][1] + m[2][1]*other[1][2];
    result[1][2] = m[0][2]*other[1][0] + m[1][2]*other[1][1] + m[2][2]*other[1][2];
    result[2][0] = m[0][0]*other[2][0] + m[1][0]*other[2][1] + m[2][0]*other[2][2];
    result[2][1] = m[0][1]*other[2][0] + m[1][1]*other[2][1] + m[2][1]*other[2][2];
    result[2][2] = m[0][2]*other[2][0] + m[1][2]*other[2][1] + m[2][2]*other[2][2];
    return result;
}

Vector3f Matrix3f::operator*(const Vector3f& other) const {
    Vector3f result;
    result.x = m[0][0]*other.x + m[1][0]*other.y + m[2][0]*other.z;
    result.y = m[0][1]*other.x + m[1][1]*other.y + m[2][1]*other.z;
    result.z = m[0][2]*other.x + m[1][2]*other.y + m[2][2]*other.z;
    return result;
}

Matrix3f & Matrix3f::operator*=(const Matrix3f& other) {
    return *this = operator*(other);
}

Matrix3f inverse(const Matrix3f& m) {
    float a =   m[1][1] * m[2][2] - m[2][1] * m[1][2];
    float b = -(m[0][1] * m[2][2] - m[2][1] * m[0][2]);
    float c =   m[0][1] * m[1][2] - m[1][1] * m[0][2];
    float d = -(m[1][0] * m[2][2] - m[2][0] * m[1][2]);
    float e =   m[0][0] * m[2][2] - m[2][0] * m[0][2];
    float f = -(m[0][0] * m[1][2] - m[1][0] * m[0][2]);
    float g =   m[1][0] * m[2][1] - m[2][0] * m[1][1];
    float h = -(m[0][0] * m[2][1] - m[2][0] * m[0][1]);
    float i =   m[0][0] * m[1][1] - m[1][0] * m[0][1];

    float det = m[0][0] * a + m[1][0] * b + m[2][0] * c;
    if (det == 0.f)
        return Matrix3f { };

    float invDet = 1.0f / det;

    Matrix3f res;
    res[0][0] = invDet * a;
    res[0][1] = invDet * b;
    res[0][2] = invDet * c;
    res[1][0] = invDet * d;
    res[1][1] = invDet * e;
    res[1][2] = invDet * f;
    res[2][0] = invDet * g;
    res[2][1] = invDet * h;
    res[2][2] = invDet * i;

    return res;
}

bool Matrix3f::operator==(const Matrix3f& other) const {
    return
        m[0][0] == other[0][0] &&
        m[0][1] == other[0][1] &&
        m[0][2] == other[0][2] &&
        m[1][0] == other[1][0] &&
        m[1][1] == other[1][1] &&
        m[1][2] == other[1][2] &&
        m[2][0] == other[2][0] &&
        m[2][1] == other[2][1] &&
        m[2][2] == other[2][2];
}

bool Matrix3f::operator!=(const Matrix3f& other) const {
    return !operator==(other);
}

// Special rectangle value such that for any X: (UNSPECIFIED_BOUNDS | X) == X
const Rectangle UNSPECIFIED_BOUNDS = { 0x20000000, 0x20000000, -0x40000000, -0x40000000 };
// Special rectangle value such that for any X: (INFINITE_BOUNDS & X) == X
const Rectangle INFINITE_BOUNDS = { -0x20000000, -0x20000000, 0x40000000, 0x40000000 };

template <typename T>
bool TRectangle<T>::contains(T x, T y) const {
    return x >= l && x < l+w && y >= t && y < t+h;
}

template <typename T>
TRectangle<T> TRectangle<T>::operator+(const TRectangle<T>& other) const {
    return { l + other.l, t + other.t, other.w, other.h };
}

template <typename T>
TRectangle<T> TRectangle<T>::operator-(const TRectangle<T>& other) const {
    return { l - other.l, t - other.t, w, h };
}

template <typename T>
TRectangle<T> TRectangle<T>::operator&(const TRectangle<T>& other) const {
    // The first two conditions are there so that floating point values are unchanged by (l+w-l)
    TRectangle<T> result;
    result.l = std::max(l, other.l);
    result.t = std::max(t, other.t);
    if (l >= other.l && l+w <= other.l+other.w)
        result.w = w;
    else if (other.l >= l && other.l+other.w <= l+w)
        result.w = other.w;
    else
        result.w = std::max(T(0), std::min(l+w, other.l+other.w)-result.l);
    if (t >= other.t && t+h <= other.t+other.h)
        result.h = h;
    else if (other.t >= t && other.t+other.h <= t+h)
        result.h = other.h;
    else
        result.h = std::max(T(0), std::min(t + h, other.t + other.h) - result.t);
    return result;
}

template <typename T>
TRectangle<T> TRectangle<T>::operator|(const TRectangle<T>& other) const {
    // The first two conditions are there so that floating point values are unchanged by (l+w-l)
    TRectangle<T> result;
    result.l = std::min(l, other.l);
    result.t = std::min(t, other.t);
    if (l <= other.l && l+w >= other.l+other.w)
        result.w = w;
    else if (other.l <= l && other.l+other.w >= l+w)
        result.w = other.w;
    else
        result.w = std::max(l+w, other.l+other.w)-result.l;
    if (t <= other.t && t+h >= other.t+other.h)
        result.h = h;
    else if (other.t <= t && other.t+other.h >= t+h)
        result.h = other.h;
    else
        result.h = std::max(t+h, other.t+other.h)-result.t;
    return result;
}

template <typename T>
TRectangle<T> TRectangle<T>::operator+(const Vector2<T>& v) const {
    return { l+v.x, t+v.y, w, h };
}

template <typename T>
TRectangle<T> TRectangle<T>::operator-(const Vector2<T>& v) const {
    return { l-v.x, t-v.y, w, h };
}

template <typename T>
TRectangle<T> TRectangle<T>::operator*(T coeff) const {
    return { l * coeff, t * coeff, w * coeff, h * coeff };
}

template <typename T>
bool TRectangle<T>::operator==(const TRectangle<T>& other) const {
    return l == other.l && t == other.t && w == other.w && h == other.h;
}

template <typename T>
bool TRectangle<T>::operator!=(const TRectangle<T>& other) const {
    return !(l == other.l && t == other.t && w == other.w && h == other.h);
}

template <typename T>
TRectangle<T>& TRectangle<T>::operator+=(const TRectangle<T>& other) {
    return *this = *this+other;
}

template <typename T>
TRectangle<T>& TRectangle<T>::operator-=(const TRectangle<T>& other) {
    return *this = *this-other;
}

template <typename T>
TRectangle<T>& TRectangle<T>::operator&=(const TRectangle<T>& other) {
    return *this = *this&other;
}

template <typename T>
TRectangle<T>& TRectangle<T>::operator|=(const TRectangle<T>& other) {
    return *this = *this|other;
}

template <typename T>
TRectangle<T>& TRectangle<T>::operator+=(const Vector2<T>& v) {
    l += v.x, t += v.y;
    return *this;
}

template <typename T>
TRectangle<T>& TRectangle<T>::operator-=(const Vector2<T>& v) {
    l -= v.x, t -= v.y;
    return *this;
}

template <typename T>
TRectangle<T>& TRectangle<T>::operator*=(T coeff) {
    return *this = *this * coeff;
}

template <typename T>
TRectangle<T>::operator bool() const {
    return w > 0 && h > 0;
}

template <typename T>
TRectangle<T> TRectangle<T>::canonical() const {
    return operator bool() ? *this : TRectangle<T>();
}

template <typename T>
Vector2<T> TRectangle<T>::dimensions() const {
    return Vector2<T> { w, h };
}

template struct TRectangle<int>;
template struct TRectangle<float>;

Rectangle translateRect(const Rectangle& r, const Vector2f& offset)
{
    auto res = r;

    res.l = static_cast<int>(std::round(res.l + offset.x));
    res.t = static_cast<int>(std::round(res.t + offset.y));

    return res;
}

Color& Color::operator+=(const Color& that)
{
    r = clamp(r + that.r, 0.f, 1.f);
    g = clamp(g + that.g, 0.f, 1.f);
    b = clamp(b + that.b, 0.f, 1.f);
    a = clamp(a + that.a, 0.f, 1.f);
    return *this;
}

Color Color::operator+(const Color& that) const
{
    auto res = *this;
    res += that;
    return res;
}

const Color Color::TRANSPARENT = { };

} // namespace compat
} // namespace textify
