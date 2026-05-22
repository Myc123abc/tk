#pragma once

#include <initializer_list>
#include <numbers>
#include <immintrin.h>
#include <assert.h>
#include <cmath>

namespace tk {

using uint8  = uint8_t;
using uint16 = uint16_t;
using uint   = uint32_t;
using uint64 = uint64_t;

template <typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template <size_t N, Numeric T>
struct vec;

////////////////////////////////////////////////////////////////////////////////
///                               float2
////////////////////////////////////////////////////////////////////////////////

template <Numeric T>
struct vec<2, T>
{
  using self = vec<2, T>;

  T x{}, y{};

  constexpr vec() noexcept = default;

  template <Numeric U>
  explicit constexpr vec(U v) noexcept
    : x(static_cast<T>(v)), y(static_cast<T>(v)) {}

  template <Numeric U>
  constexpr vec(vec<2, U> v) noexcept
    : x(static_cast<T>(v.x)), y(static_cast<T>(v.y)) {}

  template <Numeric X, Numeric Y>
  constexpr vec(X x, Y y) noexcept
    : x(static_cast<T>(x)), y(static_cast<T>(y)) {}

  constexpr auto operator+(T v) const noexcept -> self { return { x + v, y + v }; }
  constexpr auto operator-(T v) const noexcept -> self { return { x - v, y - v }; }
  constexpr auto operator*(T v) const noexcept -> self { return { x * v, y * v }; }
  constexpr auto operator/(T v) const noexcept -> self { return { x / v, y / v }; }

  constexpr auto operator+=(T v) noexcept -> self& { x += v; y += v; return *this; }
  constexpr auto operator-=(T v) noexcept -> self& { x -= v; y -= v; return *this; }
  constexpr auto operator*=(T v) noexcept -> self& { x *= v; y *= v; return *this; }
  constexpr auto operator/=(T v) noexcept -> self& { x /= v; y /= v; return *this; }

  constexpr auto operator+(self v) const noexcept -> self { return { x + v.x, y + v.y }; }
  constexpr auto operator-(self v) const noexcept -> self { return { x - v.x, y - v.y }; }
  constexpr auto operator*(self v) const noexcept -> self { return { x * v.x, y * v.y }; }
  constexpr auto operator/(self v) const noexcept -> self { return { x / v.x, y / v.y }; }

  constexpr auto operator+=(self v) noexcept -> self& { x += v.x; y += v.y; return *this; }
  constexpr auto operator-=(self v) noexcept -> self& { x -= v.x; y -= v.y; return *this; }
  constexpr auto operator*=(self v) noexcept -> self& { x *= v.x; y *= v.y; return *this; }
  constexpr auto operator/=(self v) noexcept -> self& { x /= v.x; y /= v.y; return *this; }

  constexpr auto operator-() const noexcept -> self { return { -x, -y }; }

  constexpr auto operator==(self v) const noexcept -> bool { return x == v.x && y == v.y; }
  constexpr auto operator!=(self v) const noexcept -> bool { return x != v.x || y != v.y; }
};

template <Numeric T>
inline constexpr auto dot(vec<2, T> lhs, vec<2, T> rhs) noexcept -> T { return lhs.x * rhs.x + lhs.y * rhs.y; }

template <typename T>
struct is_vec2 : std::false_type {};

template <Numeric T>
struct is_vec2<vec<2, T>> : std::true_type {};

template <typename T>
concept Vec2 = is_vec2<std::remove_cvref_t<T>>::value;

////////////////////////////////////////////////////////////////////////////////
///                               float3
////////////////////////////////////////////////////////////////////////////////

template <Numeric T>
struct vec<3, T>
{
  using self = vec<3, T>;

  T x{}, y{}, z{};

  constexpr vec() noexcept = default;

  template <Numeric U>
  explicit constexpr vec(U v) noexcept
    : x(static_cast<T>(v)), y(static_cast<T>(v)), z(static_cast<T>(v)) {}

  template <Numeric U>
  constexpr vec(vec<3, U> v) noexcept
    : x(static_cast<T>(v.x)), y(static_cast<T>(v.y)), z(static_cast<T>(v.z)) {}

  template <Numeric X, Numeric Y, Numeric Z>
  constexpr vec(X x, Y y, Z z) noexcept
    : x(static_cast<T>(x)), y(static_cast<T>(y)), z(static_cast<T>(z)) {}

  template <Numeric U, Numeric Z>
  constexpr vec(vec<2, U> xy, Z z) noexcept
    : x(static_cast<T>(xy.x)), y(static_cast<T>(xy.y)), z(static_cast<T>(z)) {}

  template <Numeric X, Numeric U>
  constexpr vec(X x, vec<2, U> yz) noexcept
    : x(static_cast<T>(x)), y(static_cast<T>(yz.x)), z(static_cast<T>(yz.y)) {}

  template <Numeric U>
  constexpr vec(std::initializer_list<U> values) noexcept
  {
    assert(values.size() <= 3);
    auto it = values.begin();
    if (it != values.end()) x = static_cast<T>(*it++);
    if (it != values.end()) y = static_cast<T>(*it++);
    if (it != values.end()) z = static_cast<T>(*it++);
  }

  constexpr auto operator+(T v) const noexcept -> self { return { x + v, y + v, z + v }; }
  constexpr auto operator-(T v) const noexcept -> self { return { x - v, y - v, z - v }; }
  constexpr auto operator*(T v) const noexcept -> self { return { x * v, y * v, z * v }; }
  constexpr auto operator/(T v) const noexcept -> self { return { x / v, y / v, z / v }; }

  constexpr auto operator+=(T v) noexcept -> self& { x += v; y += v; z += v; return *this; }
  constexpr auto operator-=(T v) noexcept -> self& { x -= v; y -= v; z -= v; return *this; }
  constexpr auto operator*=(T v) noexcept -> self& { x *= v; y *= v; z *= v; return *this; }
  constexpr auto operator/=(T v) noexcept -> self& { x /= v; y /= v; z /= v; return *this; }

  constexpr auto operator+(self v) const noexcept -> self { return { x + v.x, y + v.y, z + v.z }; }
  constexpr auto operator-(self v) const noexcept -> self { return { x - v.x, y - v.y, z - v.z }; }
  constexpr auto operator*(self v) const noexcept -> self { return { x * v.x, y * v.y, z * v.z }; }
  constexpr auto operator/(self v) const noexcept -> self { return { x / v.x, y / v.y, z / v.z }; }

  constexpr auto operator+=(self v) noexcept -> self& { x += v.x; y += v.y; z += v.z; return *this; }
  constexpr auto operator-=(self v) noexcept -> self& { x -= v.x; y -= v.y; z -= v.z; return *this; }
  constexpr auto operator*=(self v) noexcept -> self& { x *= v.x; y *= v.y; z *= v.z; return *this; }
  constexpr auto operator/=(self v) noexcept -> self& { x /= v.x; y /= v.y; z /= v.z; return *this; }

  constexpr auto operator-() const noexcept -> self { return { -x, -y, -z }; }

  constexpr auto operator==(self v) const noexcept -> bool { return x == v.x && y == v.y && z == v.z; }
  constexpr auto operator!=(self v) const noexcept -> bool { return x != v.x || y != v.y || z != v.z; }
};

template <Numeric T>
inline constexpr auto dot(vec<3, T> lhs, vec<3, T> rhs) noexcept -> T { return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z; }

////////////////////////////////////////////////////////////////////////////////
///                               float4
////////////////////////////////////////////////////////////////////////////////

template <Numeric T>
struct vec<4, T>
{
  using self = vec<4, T>;

  T x{}, y{}, z{}, w{};

  constexpr vec() noexcept = default;

  template <Numeric U>
  explicit constexpr vec(U v) noexcept
    : x(static_cast<T>(v)), y(static_cast<T>(v)), z(static_cast<T>(v)), w(static_cast<T>(v)) {}

  template <Numeric U>
  constexpr vec(vec<4, U> v) noexcept
    : x(static_cast<T>(v.x)), y(static_cast<T>(v.y)), z(static_cast<T>(v.z)), w(static_cast<T>(v.w)) {}

  template <Numeric X, Numeric Y, Numeric Z, Numeric W>
  constexpr vec(X x, Y y, Z z, W w) noexcept
    : x(static_cast<T>(x)), y(static_cast<T>(y)), z(static_cast<T>(z)), w(static_cast<T>(w)) {}

  template <Numeric U, Numeric V>
  constexpr vec(vec<2, U> xy, vec<2, V> zw) noexcept
    : x(static_cast<T>(xy.x)), y(static_cast<T>(xy.y)), z(static_cast<T>(zw.x)), w(static_cast<T>(zw.y)) {}

  template <Numeric U, Numeric W>
  constexpr vec(vec<3, U> xyz, W w) noexcept
    : x(static_cast<T>(xyz.x)), y(static_cast<T>(xyz.y)), z(static_cast<T>(xyz.z)), w(static_cast<T>(w)) {}

  template <Numeric X, Numeric U>
  constexpr vec(X x, vec<3, U> yzw) noexcept
    : x(static_cast<T>(x)), y(static_cast<T>(yzw.x)), z(static_cast<T>(yzw.y)), w(static_cast<T>(yzw.z)) {}

  template <Numeric U, Numeric Z, Numeric W>
  constexpr vec(vec<2, U> xy, Z z, W w) noexcept
    : x(static_cast<T>(xy.x)), y(static_cast<T>(xy.y)), z(static_cast<T>(z)), w(static_cast<T>(w)) {}

  template <Numeric X, Numeric U, Numeric W>
  constexpr vec(X x, vec<2, U> yz, W w) noexcept
    : x(static_cast<T>(x)), y(static_cast<T>(yz.x)), z(static_cast<T>(yz.y)), w(static_cast<T>(w)) {}

  template <Numeric X, Numeric Y, Numeric U>
  constexpr vec(X x, Y y, vec<2, U> zw) noexcept
    : x(static_cast<T>(x)), y(static_cast<T>(y)), z(static_cast<T>(zw.x)), w(static_cast<T>(zw.y)) {}

  template <Numeric U>
  constexpr vec(std::initializer_list<U> values) noexcept
  {
    assert(values.size() <= 4);
    auto it = values.begin();
    if (it != values.end()) x = static_cast<T>(*it++);
    if (it != values.end()) y = static_cast<T>(*it++);
    if (it != values.end()) z = static_cast<T>(*it++);
    if (it != values.end()) w = static_cast<T>(*it++);
  }

  constexpr auto operator+(T v) const noexcept -> self { return { x + v, y + v, z + v, w + v }; }
  constexpr auto operator-(T v) const noexcept -> self { return { x - v, y - v, z - v, w - v }; }
  constexpr auto operator*(T v) const noexcept -> self { return { x * v, y * v, z * v, w * v }; }
  constexpr auto operator/(T v) const noexcept -> self { return { x / v, y / v, z / v, w / v }; }

  constexpr auto operator+=(T v) noexcept -> self& { x += v; y += v; z += v; w += v; return *this; }
  constexpr auto operator-=(T v) noexcept -> self& { x -= v; y -= v; z -= v; w -= v; return *this; }
  constexpr auto operator*=(T v) noexcept -> self& { x *= v; y *= v; z *= v; w *= v; return *this; }
  constexpr auto operator/=(T v) noexcept -> self& { x /= v; y /= v; z /= v; w /= v; return *this; }

  constexpr auto operator+(self v) const noexcept -> self { return { x + v.x, y + v.y, z + v.z, w + v.w }; }
  constexpr auto operator-(self v) const noexcept -> self { return { x - v.x, y - v.y, z - v.z, w - v.w }; }
  constexpr auto operator*(self v) const noexcept -> self { return { x * v.x, y * v.y, z * v.z, w * v.w }; }
  constexpr auto operator/(self v) const noexcept -> self { return { x / v.x, y / v.y, z / v.z, w / v.w }; }

  constexpr auto operator+=(self v) noexcept -> self& { x += v.x; y += v.y; z += v.z; w += v.w; return *this; }
  constexpr auto operator-=(self v) noexcept -> self& { x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }
  constexpr auto operator*=(self v) noexcept -> self& { x *= v.x; y *= v.y; z *= v.z; w *= v.w; return *this; }
  constexpr auto operator/=(self v) noexcept -> self& { x /= v.x; y /= v.y; z /= v.z; w /= v.w; return *this; }

  constexpr auto operator-() const noexcept -> self { return { -x, -y, -z, -w }; }

  constexpr auto operator==(self v) const noexcept -> bool { return x == v.x && y == v.y && z == v.z && w == v.w; }
  constexpr auto operator!=(self v) const noexcept -> bool { return x != v.x || y != v.y || z != v.z || w != v.w; }
};

template <Numeric T>
inline constexpr auto dot(vec<4, T> lhs, vec<4, T> rhs) noexcept -> T { return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w; }

////////////////////////////////////////////////////////////////////////////////
///                               float4 simd
////////////////////////////////////////////////////////////////////////////////

template <>
struct alignas(16) vec<4, float>
{
  using self = vec<4, float>;

  union
  {
    struct { float x{}, y{}, z{}, w{}; };
    __m128 simd;
  };

  constexpr vec(std::initializer_list<float> values) noexcept
  {
    assert(values.size() <= 4);
    auto it = values.begin();
    if (it != values.end()) x = *it++;
    if (it != values.end()) y = *it++;
    if (it != values.end()) z = *it++;
    if (it != values.end()) w = *it++;
  }

  vec() noexcept : simd(_mm_setzero_ps()) {}

  vec(float x, float y, float z, float w) noexcept
    : simd(_mm_set_ps(w, z, y, x)) {}

  explicit vec(float v) noexcept
    : simd(_mm_set1_ps(v)) {}

  template <Numeric U, Numeric Z, Numeric W>
  vec(vec<2, U> xy, Z z, W w) noexcept
    : simd(_mm_set_ps(
        (float)w,
        (float)z,
        (float)xy.y,
        (float)xy.x)) {}

  template <Numeric U, Numeric W>
  vec(vec<3, U> xyz, W w) noexcept
    : simd(_mm_set_ps(
        (float)w,
        (float)xyz.z,
        (float)xyz.y,
        (float)xyz.x)) {}

  template <Numeric X, Numeric U>
  vec(X x, vec<3, U> yzw) noexcept
    : simd(_mm_set_ps(
        (float)yzw.z,
        (float)yzw.y,
        (float)yzw.x,
        (float)x)) {}

  template <Numeric U, Numeric V>
  vec(vec<2, U> xy, vec<2, V> zw) noexcept
    : simd(_mm_set_ps(
        (float)zw.y,
        (float)zw.x,
        (float)xy.y,
        (float)xy.x)) {}

  vec(__m128 v) noexcept : simd(v) {}

  auto operator+(float v) const noexcept -> self { return _mm_add_ps(simd, _mm_set1_ps(v)); }
  auto operator-(float v) const noexcept -> self { return _mm_sub_ps(simd, _mm_set1_ps(v)); }
  auto operator*(float v) const noexcept -> self { return _mm_mul_ps(simd, _mm_set1_ps(v)); }
  auto operator/(float v) const noexcept -> self { return _mm_div_ps(simd, _mm_set1_ps(v)); }

  auto operator+=(float v) noexcept -> self& { simd = _mm_add_ps(simd, _mm_set1_ps(v)); return *this; }
  auto operator-=(float v) noexcept -> self& { simd = _mm_sub_ps(simd, _mm_set1_ps(v)); return *this; }
  auto operator*=(float v) noexcept -> self& { simd = _mm_mul_ps(simd, _mm_set1_ps(v)); return *this; }
  auto operator/=(float v) noexcept -> self& { simd = _mm_div_ps(simd, _mm_set1_ps(v)); return *this; }

  auto operator+(self v) const noexcept -> self { return _mm_add_ps(simd, v.simd); }
  auto operator-(self v) const noexcept -> self { return _mm_sub_ps(simd, v.simd); }
  auto operator*(self v) const noexcept -> self { return _mm_mul_ps(simd, v.simd); }
  auto operator/(self v) const noexcept -> self { return _mm_div_ps(simd, v.simd); }

  auto operator+=(self v) noexcept -> self& { simd = _mm_add_ps(simd, v.simd); return *this; }
  auto operator-=(self v) noexcept -> self& { simd = _mm_sub_ps(simd, v.simd); return *this; }
  auto operator*=(self v) noexcept -> self& { simd = _mm_mul_ps(simd, v.simd); return *this; }
  auto operator/=(self v) noexcept -> self& { simd = _mm_div_ps(simd, v.simd); return *this; }

  auto operator-() const noexcept -> self { return _mm_sub_ps(_mm_setzero_ps(), simd); }

  auto operator==(self v) const noexcept -> bool
  {
    __m128 cmp = _mm_cmpeq_ps(simd, v.simd);
    return (_mm_movemask_ps(cmp) == 0xF);
  }

  auto operator!=(self v) const noexcept -> bool { return !operator==(v); }
};

inline auto dot(vec<4, float> a, vec<4, float> b) noexcept -> float
{
  __m128 m = _mm_mul_ps(a.simd, b.simd);
  m = _mm_hadd_ps(m, m);
  m = _mm_hadd_ps(m, m);
  return _mm_cvtss_f32(m);
}

////////////////////////////////////////////////////////////////////////////////
///                               misc
////////////////////////////////////////////////////////////////////////////////

using float2 = vec<2, float>;
using float3 = vec<3, float>;
using float4 = vec<4, float>;
using int2   = vec<2, int>;
using int3   = vec<3, int>;
using int4   = vec<4, int>;
using uint2  = vec<2, uint>;
using uint3  = vec<3, uint>;
using uint4  = vec<4, uint>;

constexpr auto radians(float deg) noexcept
{
  return deg * (std::numbers::pi / 180.f);
}

constexpr auto degrees(float rad) noexcept
{
  return rad * (180.f / std::numbers::pi);
}

constexpr auto cross(float2 a, float2 b) noexcept
{
  return a.x * b.y - a.y * b.x;
}

constexpr auto cross(float2 a, float2 b, float2 c) noexcept
{
  return cross(b - a, c - a);
}

constexpr auto dot(float2 a, float2 b) noexcept
{
  return a.x * b.x + a.y * b.y;
}

constexpr auto length_sq(float2 p) noexcept
{
  return dot(p, p);
}

inline auto length(float2 p) noexcept
{
  return std::sqrt(length_sq(p));
}

inline auto normalize(float2 v) noexcept -> float2
{
  auto len = length(v);
  return len > 0.f ? float2{ v.x / len, v.y / len } : float2{};
}

constexpr auto abs(float2 v) noexcept -> float2
{
  return { std::abs(v.x), std::abs(v.y) };
}

constexpr auto floor(float2 v) noexcept -> float2
{
  return { std::floor(v.x), std::floor(v.y) };
}

}
