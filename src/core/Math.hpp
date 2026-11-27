#pragma once
// Matematica minima (estilo GLSL) para no depender de GLM en el motor.
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace xeldi {

struct Vec3 {
    float x = 0, y = 0, z = 0;
    constexpr Vec3() = default;
    constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    constexpr Vec3 operator+(Vec3 o) const { return {x + o.x, y + o.y, z + o.z}; }
    constexpr Vec3 operator-(Vec3 o) const { return {x - o.x, y - o.y, z - o.z}; }
    constexpr Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    constexpr Vec3 operator-() const { return {-x, -y, -z}; }
    Vec3& operator+=(Vec3 o) { x += o.x; y += o.y; z += o.z; return *this; }
    float operator[](int i) const { return i == 0 ? x : (i == 1 ? y : z); }
    bool operator==(const Vec3&) const = default;
};

inline float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(Vec3 a, Vec3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
inline float length(Vec3 a) { return std::sqrt(dot(a, a)); }
inline Vec3 normalize(Vec3 a) {
    float l = length(a);
    return l > 1e-8f ? a * (1.0f / l) : Vec3{};
}

struct IVec3 {
    int x = 0, y = 0, z = 0;
    constexpr IVec3() = default;
    constexpr IVec3(int x_, int y_, int z_) : x(x_), y(y_), z(z_) {}
    constexpr IVec3 operator+(IVec3 o) const { return {x + o.x, y + o.y, z + o.z}; }
    constexpr IVec3 operator-(IVec3 o) const { return {x - o.x, y - o.y, z - o.z}; }
    bool operator==(const IVec3&) const = default;
    Vec3 toVec3() const { return {float(x), float(y), float(z)}; }
};

struct IVec3Hash {
    std::size_t operator()(const IVec3& v) const noexcept {
        std::size_t h = std::size_t(std::uint32_t(v.x)) * 73856093u;
        h ^= std::size_t(std::uint32_t(v.y)) * 19349663u;
        h ^= std::size_t(std::uint32_t(v.z)) * 83492791u;
        return h;
    }
};

inline IVec3 floorToCell(Vec3 p) {
    return {int(std::floor(p.x)), int(std::floor(p.y)), int(std::floor(p.z))};
}

struct Vec4 {
    float x = 0, y = 0, z = 0, w = 0;
};

/// Matriz 4x4 column-major (m[col][row]) como en OpenGL.
struct Mat4 {
    std::array<std::array<float, 4>, 4> m{};

    static Mat4 identity() {
        Mat4 r;
        for (int i = 0; i < 4; ++i) r.m[i][i] = 1;
        return r;
    }
    Mat4 operator*(const Mat4& o) const {
        Mat4 r;
        for (int c = 0; c < 4; ++c)
            for (int rr = 0; rr < 4; ++rr) {
                float s = 0;
                for (int k = 0; k < 4; ++k) s += m[k][rr] * o.m[c][k];
                r.m[c][rr] = s;
            }
        return r;
    }
    Vec4 operator*(const Vec4& v) const {
        return {m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z + m[3][0] * v.w,
                m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1] * v.w,
                m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2] * v.w,
                m[0][3] * v.x + m[1][3] * v.y + m[2][3] * v.z + m[3][3] * v.w};
    }
    Vec4 transform(Vec3 p) const { return (*this) * Vec4{p.x, p.y, p.z, 1.0f}; }

    static Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up) {
        Vec3 f = normalize(target - eye);
        Vec3 s = normalize(cross(f, up));
        Vec3 u = cross(s, f);
        Mat4 r = identity();
        r.m[0][0] = s.x; r.m[1][0] = s.y; r.m[2][0] = s.z;
        r.m[0][1] = u.x; r.m[1][1] = u.y; r.m[2][1] = u.z;
        r.m[0][2] = -f.x; r.m[1][2] = -f.y; r.m[2][2] = -f.z;
        r.m[3][0] = -dot(s, eye);
        r.m[3][1] = -dot(u, eye);
        r.m[3][2] = dot(f, eye);
        return r;
    }
    static Mat4 perspective(float fovYRad, float aspect, float zNear, float zFar) {
        float t = std::tan(fovYRad / 2);
        Mat4 r;
        r.m[0][0] = 1.0f / (aspect * t);
        r.m[1][1] = 1.0f / t;
        r.m[2][2] = -(zFar + zNear) / (zFar - zNear);
        r.m[2][3] = -1;
        r.m[3][2] = -(2 * zFar * zNear) / (zFar - zNear);
        return r;
    }
    static Mat4 ortho(float l, float r_, float b, float t, float n, float f) {
        Mat4 r = identity();
        r.m[0][0] = 2 / (r_ - l);
        r.m[1][1] = 2 / (t - b);
        r.m[2][2] = -2 / (f - n);
        r.m[3][0] = -(r_ + l) / (r_ - l);
        r.m[3][1] = -(t + b) / (t - b);
        r.m[3][2] = -(f + n) / (f - n);
        return r;
    }
};

constexpr float kPi = 3.14159265358979323846f;
inline float radians(float deg) { return deg * kPi / 180.0f; }

}  // namespace xeldi
