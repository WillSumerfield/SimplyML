#pragma once
#include <algorithm>
#include <cmath>

namespace sml
{

template<typename T>
inline constexpr T PiV = static_cast<T>(3.141592653589793238462643383279502884L);

inline constexpr float Pi    = PiV<float>;
inline constexpr float TwoPi = 2.0f * Pi;

// Scalars ------------------------------------------------------------------------------------------

template<typename T>
constexpr T sign(T v)
{
    return v < T{0} ? T{-1} : T{1};
}

template<typename T>
constexpr T radToDeg(T r)
{
    return r * (T{180} / PiV<T>);
}

template<typename T>
constexpr T degToRad(T d)
{
    return d * (PiV<T> / T{180});
}

template<typename T>
T mix(T const& a, T const& b, float ratio)
{
    return a + (b - a) * ratio;
}

inline float sigmoid(float x)
{
    return 1.0f / (1.0f + std::exp(-x));
}

inline float gaussian(float x, float a, float b, float c)
{
    float const n = x - b;
    return a * std::exp(-(n * n) / (2.0f * c * c));
}

/// Clamps |x| to `maximum`, keeping the sign.
template<typename T>
T clampAmplitude(T x, T maximum)
{
    return sign(x) * std::min(std::abs(x), maximum);
}

// 2D vectors: any type with `.x`/`.y` that is brace-constructible from two components --------------

template<typename V>
constexpr auto dot(V const& a, V const& b)
{
    return a.x * b.x + a.y * b.y;
}

template<typename V>
constexpr auto cross(V const& a, V const& b)
{
    return a.x * b.y - a.y * b.x;
}

template<typename V>
constexpr auto length2(V const& v)
{
    return dot(v, v);
}

template<typename V>
auto length(V const& v)
{
    return std::sqrt(length2(v));
}

/// Signed angle from `a` to `b`, in radians.
template<typename V>
auto angle(V const& a, V const& b)
{
    return std::atan2(cross(a, b), dot(a, b));
}

/// Left-hand perpendicular.
template<typename V>
constexpr V normal(V const& v)
{
    return V{-v.y, v.x};
}

/// Unit vector, or the zero vector when `v` is zero.
template<typename V>
V normalize(V const& v)
{
    auto const l = length(v);
    if (l == decltype(l){0}) {
        return V{};
    }
    return V{v.x / l, v.y / l};
}

template<typename V>
V rotate(V const& v, float angle)
{
    float const c = std::cos(angle);
    float const s = std::sin(angle);
    return V{c * v.x - s * v.y, s * v.x + c * v.y};
}

/// Rotates `v` by the rotation that maps (1, 0) onto the unit vector `dir`.
template<typename V>
constexpr V rotateDir(V const& v, V const& dir)
{
    return V{dir.x * v.x - dir.y * v.y, dir.y * v.x + dir.x * v.y};
}

/// Reflects `v` about the unit normal `n`.
template<typename V>
constexpr V reflect(V const& v, V const& n)
{
    auto const d = 2 * dot(v, n);
    return V{v.x - n.x * d, v.y - n.y * d};
}

} // namespace sml
