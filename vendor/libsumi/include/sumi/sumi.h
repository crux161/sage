/**
 * @file sumi.h
 * @brief Sumi (墨) - A lightweight, Cross-Platform (CUDA/Metal/CPU) graphics math library.
 * @version 1.1.0
 * @namespace sumi
 */

#pragma once

// This header handles the compiler detection (NVCC vs Metal vs Clang/MSVC)
// and defines the SUMI_CTX macros and native type aliases.
#include "sumi_platform.h"

#include <cmath>
#include <algorithm>
#include <iostream>
#include <cstdint>
#include <limits>

// Constant Definitions
#ifndef SUMI_PI
#define SUMI_PI 3.14159265358979323846f
#endif

#ifndef SUMI_EPSILON
#define SUMI_EPSILON 1.192092896e-07F
#endif

namespace sumi {

// =================================================================================================
// Forward Declarations
// =================================================================================================
#if !SUMI_USE_NATIVE_TYPES
    struct bvec2;
    struct bvec3;
    struct bvec4;
    struct vec2;
    struct vec3;
    struct vec4;
    struct mat2;
    struct mat3;
    struct mat4;
    struct quat;
#endif

struct Sampler2D;

// =================================================================================================
// TYPE DEFINITIONS (CPU / CUDA)
// =================================================================================================
// If Metal (SUMI_USE_NATIVE_TYPES) is active, these structs are skipped 
// because they are typedef'd to native SIMD types in sumi_platform.h
// =================================================================================================

#if !SUMI_USE_NATIVE_TYPES

struct bvec2 {
    bool x, y;

    SUMI_CTX bvec2() : x(false), y(false) {}
    SUMI_CTX explicit bvec2(bool s) : x(s), y(s) {}
    SUMI_CTX bvec2(bool _x, bool _y) : x(_x), y(_y) {}

    SUMI_CTX bool& operator[](int i) { return (&x)[i]; }
    SUMI_CTX const bool& operator[](int i) const { return (&x)[i]; }
};

struct bvec3 {
    bool x, y, z;

    SUMI_CTX bvec3() : x(false), y(false), z(false) {}
    SUMI_CTX explicit bvec3(bool s) : x(s), y(s), z(s) {}
    SUMI_CTX bvec3(bool _x, bool _y, bool _z) : x(_x), y(_y), z(_z) {}
    SUMI_CTX bvec3(const bvec2& v, bool _z) : x(v.x), y(v.y), z(_z) {}

    SUMI_CTX bool& operator[](int i) { return (&x)[i]; }
    SUMI_CTX const bool& operator[](int i) const { return (&x)[i]; }
};

struct bvec4 {
    bool x, y, z, w;

    SUMI_CTX bvec4() : x(false), y(false), z(false), w(false) {}
    SUMI_CTX explicit bvec4(bool s) : x(s), y(s), z(s), w(s) {}
    SUMI_CTX bvec4(bool _x, bool _y, bool _z, bool _w) : x(_x), y(_y), z(_z), w(_w) {}
    SUMI_CTX bvec4(const bvec3& v, bool _w) : x(v.x), y(v.y), z(v.z), w(_w) {}
    SUMI_CTX bvec4(const bvec2& v, bool _z, bool _w) : x(v.x), y(v.y), z(_z), w(_w) {}

    SUMI_CTX bool& operator[](int i) { return (&x)[i]; }
    SUMI_CTX const bool& operator[](int i) const { return (&x)[i]; }
};

struct vec2 {
    union { float x, r, s; };
    union { float y, g, t; };

    SUMI_CTX vec2() : x(0.0f), y(0.0f) {}
    SUMI_CTX explicit vec2(float s) : x(s), y(s) {}
    SUMI_CTX vec2(float _x, float _y) : x(_x), y(_y) {}
    
    // Accessors
    SUMI_CTX float& operator[](int i) { return (&x)[i]; }
    SUMI_CTX const float& operator[](int i) const { return (&x)[i]; }

    // Unary
    SUMI_CTX vec2 operator-() const { return vec2(-x, -y); }

    // Operators
    SUMI_CTX vec2 operator+(const vec2& v) const { return vec2(x+v.x, y+v.y); }
    SUMI_CTX vec2 operator-(const vec2& v) const { return vec2(x-v.x, y-v.y); }
    SUMI_CTX vec2 operator*(const vec2& v) const { return vec2(x*v.x, y*v.y); }
    SUMI_CTX vec2 operator/(const vec2& v) const { return vec2(x/v.x, y/v.y); }
    SUMI_CTX vec2 operator+(float s) const { return vec2(x+s, y+s); }
    SUMI_CTX vec2 operator-(float s) const { return vec2(x-s, y-s); }
    SUMI_CTX vec2 operator*(float s) const { return vec2(x*s, y*s); }
    SUMI_CTX vec2 operator/(float s) const { return vec2(x/s, y/s); }
    SUMI_CTX bool operator==(const vec2& v) const { return x == v.x && y == v.y; }
    SUMI_CTX bool operator!=(const vec2& v) const { return !(*this == v); }

    // Assignment
    SUMI_CTX vec2& operator+=(const vec2& v) { x+=v.x; y+=v.y; return *this; }
    SUMI_CTX vec2& operator-=(const vec2& v) { x-=v.x; y-=v.y; return *this; }
    SUMI_CTX vec2& operator*=(const vec2& v) { x*=v.x; y*=v.y; return *this; }
    SUMI_CTX vec2& operator/=(const vec2& v) { x/=v.x; y/=v.y; return *this; }
    SUMI_CTX vec2& operator+=(float s) { x+=s; y+=s; return *this; }
    SUMI_CTX vec2& operator-=(float s) { x-=s; y-=s; return *this; }
    SUMI_CTX vec2& operator*=(float s) { x*=s; y*=s; return *this; }
    SUMI_CTX vec2& operator/=(float s) { x/=s; y/=s; return *this; }

    // Swizzles (Basic)
    SUMI_CTX vec2 xy() const { return vec2(x,y); }
    SUMI_CTX vec2 yx() const { return vec2(y,x); }
    SUMI_CTX vec2 xx() const { return vec2(x,x); }
    SUMI_CTX vec2 yy() const { return vec2(y,y); }
};

struct vec3 {
    union { float x, r, s; };
    union { float y, g, t; };
    union { float z, b, p; };

    SUMI_CTX vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    SUMI_CTX explicit vec3(float s) : x(s), y(s), z(s) {}
    SUMI_CTX vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    SUMI_CTX vec3(const vec2& v) : x(v.x), y(v.y), z(0.0f) {}
    SUMI_CTX vec3(const vec2& v, float _z) : x(v.x), y(v.y), z(_z) {}
    SUMI_CTX vec3(float _x, const vec2& v) : x(_x), y(v.x), z(v.y) {}

    SUMI_CTX float& operator[](int i) { return (&x)[i]; }
    SUMI_CTX const float& operator[](int i) const { return (&x)[i]; }
    SUMI_CTX vec3 operator-() const { return vec3(-x, -y, -z); }

    SUMI_CTX vec3 operator+(const vec3& v) const { return vec3(x+v.x, y+v.y, z+v.z); }
    SUMI_CTX vec3 operator-(const vec3& v) const { return vec3(x-v.x, y-v.y, z-v.z); }
    SUMI_CTX vec3 operator*(const vec3& v) const { return vec3(x*v.x, y*v.y, z*v.z); }
    SUMI_CTX vec3 operator/(const vec3& v) const { return vec3(x/v.x, y/v.y, z/v.z); }
    SUMI_CTX vec3 operator+(float s) const { return vec3(x+s, y+s, z+s); }
    SUMI_CTX vec3 operator-(float s) const { return vec3(x-s, y-s, z-s); }
    SUMI_CTX vec3 operator*(float s) const { return vec3(x*s, y*s, z*s); }
    SUMI_CTX vec3 operator/(float s) const { return vec3(x/s, y/s, z/s); }
    SUMI_CTX bool operator==(const vec3& v) const { return x == v.x && y == v.y && z == v.z; }
    SUMI_CTX bool operator!=(const vec3& v) const { return !(*this == v); }

    SUMI_CTX vec3& operator+=(const vec3& v) { x+=v.x; y+=v.y; z+=v.z; return *this; }
    SUMI_CTX vec3& operator-=(const vec3& v) { x-=v.x; y-=v.y; z-=v.z; return *this; }
    SUMI_CTX vec3& operator*=(const vec3& v) { x*=v.x; y*=v.y; z*=v.z; return *this; }
    SUMI_CTX vec3& operator/=(const vec3& v) { x/=v.x; y/=v.y; z/=v.z; return *this; }
    SUMI_CTX vec3& operator+=(float s) { x+=s; y+=s; z+=s; return *this; }
    SUMI_CTX vec3& operator-=(float s) { x-=s; y-=s; z-=s; return *this; }
    SUMI_CTX vec3& operator*=(float s) { x*=s; y*=s; z*=s; return *this; }
    SUMI_CTX vec3& operator/=(float s) { x/=s; y/=s; z/=s; return *this; }

    // Swizzles
    SUMI_CTX vec2 xy() const { return vec2(x, y); }
    SUMI_CTX vec2 xz() const { return vec2(x, z); }
    SUMI_CTX vec2 yz() const { return vec2(y, z); }
    SUMI_CTX vec2 yx() const { return vec2(y, x); }
    SUMI_CTX vec2 zx() const { return vec2(z, x); }
    SUMI_CTX vec2 zy() const { return vec2(z, y); }
    SUMI_CTX vec3 xyz() const { return vec3(x, y, z); }
    SUMI_CTX vec3 xzy() const { return vec3(x, z, y); }
    SUMI_CTX vec3 yxz() const { return vec3(y, x, z); }
    SUMI_CTX vec3 yzx() const { return vec3(y, z, x); }
    SUMI_CTX vec3 zxy() const { return vec3(z, x, y); }
    SUMI_CTX vec3 zyx() const { return vec3(z, y, x); }
};

struct vec4 {
    union { float x, r, s; };
    union { float y, g, t; };
    union { float z, b, p; };
    union { float w, a, q; };

    SUMI_CTX vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    SUMI_CTX explicit vec4(float s) : x(s), y(s), z(s), w(s) {}
    SUMI_CTX vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
    SUMI_CTX vec4(const vec3& v, float _w) : x(v.x), y(v.y), z(v.z), w(_w) {}
    SUMI_CTX vec4(float _x, const vec3& v) : x(_x), y(v.x), z(v.y), w(v.z) {}
    SUMI_CTX vec4(const vec2& v, float _z, float _w) : x(v.x), y(v.y), z(_z), w(_w) {}
    SUMI_CTX vec4(float _x, float _y, const vec2& v) : x(_x), y(_y), z(v.x), w(v.y) {}
    SUMI_CTX vec4(const vec2& xy, const vec2& zw) : x(xy.x), y(xy.y), z(zw.x), w(zw.y) {}

    SUMI_CTX float& operator[](int i) { return (&x)[i]; }
    SUMI_CTX const float& operator[](int i) const { return (&x)[i]; }
    SUMI_CTX vec4 operator-() const { return vec4(-x, -y, -z, -w); }

    SUMI_CTX vec4 operator+(const vec4& v) const { return vec4(x+v.x, y+v.y, z+v.z, w+v.w); }
    SUMI_CTX vec4 operator-(const vec4& v) const { return vec4(x-v.x, y-v.y, z-v.z, w-v.w); }
    SUMI_CTX vec4 operator*(const vec4& v) const { return vec4(x*v.x, y*v.y, z*v.z, w*v.w); }
    SUMI_CTX vec4 operator/(const vec4& v) const { return vec4(x/v.x, y/v.y, z/v.z, w/v.w); }
    SUMI_CTX vec4 operator+(float s) const { return vec4(x+s, y+s, z+s, w+s); }
    SUMI_CTX vec4 operator-(float s) const { return vec4(x-s, y-s, z-s, w-s); }
    SUMI_CTX vec4 operator*(float s) const { return vec4(x*s, y*s, z*s, w*s); }
    SUMI_CTX vec4 operator/(float s) const { return vec4(x/s, y/s, z/s, w/s); }
    SUMI_CTX bool operator==(const vec4& v) const { return x == v.x && y == v.y && z == v.z && w == v.w; }
    SUMI_CTX bool operator!=(const vec4& v) const { return !(*this == v); }

    SUMI_CTX vec4& operator+=(const vec4& v) { x+=v.x; y+=v.y; z+=v.z; w+=v.w; return *this; }
    SUMI_CTX vec4& operator-=(const vec4& v) { x-=v.x; y-=v.y; z-=v.z; w-=v.w; return *this; }
    SUMI_CTX vec4& operator*=(const vec4& v) { x*=v.x; y*=v.y; z*=v.z; w*=v.w; return *this; }
    SUMI_CTX vec4& operator/=(const vec4& v) { x/=v.x; y/=v.y; z/=v.z; w/=v.w; return *this; }
    SUMI_CTX vec4& operator+=(float s) { x+=s; y+=s; z+=s; w+=s; return *this; }
    SUMI_CTX vec4& operator-=(float s) { x-=s; y-=s; z-=s; w-=s; return *this; }
    SUMI_CTX vec4& operator*=(float s) { x*=s; y*=s; z*=s; w*=s; return *this; }
    SUMI_CTX vec4& operator/=(float s) { x/=s; y/=s; z/=s; w/=s; return *this; }
    
    SUMI_CTX vec3 xyz() const { return vec3(x,y,z); }
    SUMI_CTX vec3 rgb() const { return vec3(x,y,z); }
    SUMI_CTX vec3 yzx() const { return vec3(y,z,x); }
    SUMI_CTX vec3 zyx() const { return vec3(z,y,x); }
    SUMI_CTX vec2 xy() const { return vec2(x,y); }
    SUMI_CTX vec2 xz() const { return vec2(x,z); }
    SUMI_CTX vec2 yz() const { return vec2(y,z); }
    SUMI_CTX vec2 zw() const { return vec2(z,w); }
};

// Global Scalar Operators (s * v, s / v, etc.)
inline SUMI_CTX vec2 operator*(float s, const vec2& v) { return vec2(v.x*s, v.y*s); }
inline SUMI_CTX vec3 operator*(float s, const vec3& v) { return vec3(v.x*s, v.y*s, v.z*s); }
inline SUMI_CTX vec4 operator*(float s, const vec4& v) { return vec4(v.x*s, v.y*s, v.z*s, v.w*s); }
inline SUMI_CTX vec2 operator/(float s, const vec2& v) { return vec2(s/v.x, s/v.y); }
inline SUMI_CTX vec3 operator/(float s, const vec3& v) { return vec3(s/v.x, s/v.y, s/v.z); }
inline SUMI_CTX vec4 operator/(float s, const vec4& v) { return vec4(s/v.x, s/v.y, s/v.z, s/v.w); }
inline SUMI_CTX vec2 operator+(float s, const vec2& v) { return vec2(s+v.x, s+v.y); }
inline SUMI_CTX vec3 operator+(float s, const vec3& v) { return vec3(s+v.x, s+v.y, s+v.z); }
inline SUMI_CTX vec4 operator+(float s, const vec4& v) { return vec4(s+v.x, s+v.y, s+v.z, s+v.w); }
inline SUMI_CTX vec2 operator-(float s, const vec2& v) { return vec2(s-v.x, s-v.y); }
inline SUMI_CTX vec3 operator-(float s, const vec3& v) { return vec3(s-v.x, s-v.y, s-v.z); }
inline SUMI_CTX vec4 operator-(float s, const vec4& v) { return vec4(s-v.x, s-v.y, s-v.z, s-v.w); }

struct mat2 {
    vec2 cols[2];
    SUMI_CTX mat2() {
        cols[0] = vec2(1,0); cols[1] = vec2(0,1);
    }
    SUMI_CTX explicit mat2(float s) {
        cols[0] = vec2(s,0); cols[1] = vec2(0,s);
    }
    SUMI_CTX mat2(const vec2& c0, const vec2& c1) {
        cols[0] = c0; cols[1] = c1;
    }
    // Column-major construction
    SUMI_CTX mat2(float x0, float y0,
                  float x1, float y1) {
        cols[0] = vec2(x0,y0); cols[1] = vec2(x1,y1);
    }
    SUMI_CTX vec2& operator[](int i) { return cols[i]; }
    SUMI_CTX const vec2& operator[](int i) const { return cols[i]; }

    SUMI_CTX mat2 operator+(const mat2& m) const { return mat2(cols[0]+m[0], cols[1]+m[1]); }
    SUMI_CTX mat2 operator-(const mat2& m) const { return mat2(cols[0]-m[0], cols[1]-m[1]); }
    SUMI_CTX mat2 operator*(float s) const { return mat2(cols[0]*s, cols[1]*s); }
    SUMI_CTX mat2 operator/(float s) const { return mat2(cols[0]/s, cols[1]/s); }
    SUMI_CTX mat2& operator+=(const mat2& m) { cols[0]+=m[0]; cols[1]+=m[1]; return *this; }
    SUMI_CTX mat2& operator-=(const mat2& m) { cols[0]-=m[0]; cols[1]-=m[1]; return *this; }
    SUMI_CTX mat2& operator*=(float s) { cols[0]*=s; cols[1]*=s; return *this; }
    SUMI_CTX mat2& operator/=(float s) { cols[0]/=s; cols[1]/=s; return *this; }
    SUMI_CTX bool operator==(const mat2& m) const { return cols[0] == m[0] && cols[1] == m[1]; }
    SUMI_CTX bool operator!=(const mat2& m) const { return !(*this == m); }

    SUMI_CTX mat2 operator*(const mat2& m) const {
        mat2 res(0.0f);
        for(int i=0; i<2; ++i) {
            for(int j=0; j<2; ++j) {
                res[i][j] = cols[0][j]*m[i][0] + cols[1][j]*m[i][1];
            }
        }
        return res;
    }
    SUMI_CTX vec2 operator*(const vec2& v) const {
        return vec2(cols[0]*v.x + cols[1]*v.y);
    }
};

struct mat3 {
    vec3 cols[3];
    SUMI_CTX mat3() {
        cols[0] = vec3(1,0,0); cols[1] = vec3(0,1,0); cols[2] = vec3(0,0,1);
    }
    SUMI_CTX explicit mat3(float s) {
        cols[0] = vec3(s,0,0); cols[1] = vec3(0,s,0); cols[2] = vec3(0,0,s);
    }
    SUMI_CTX mat3(const vec3& c0, const vec3& c1, const vec3& c2) {
        cols[0] = c0; cols[1] = c1; cols[2] = c2;
    }
    // Column-major construction
    SUMI_CTX mat3(float x0, float y0, float z0,
                  float x1, float y1, float z1,
                  float x2, float y2, float z2) {
        cols[0] = vec3(x0,y0,z0); cols[1] = vec3(x1,y1,z1); cols[2] = vec3(x2,y2,z2);
    }
    SUMI_CTX vec3& operator[](int i) { return cols[i]; }
    SUMI_CTX const vec3& operator[](int i) const { return cols[i]; }

    SUMI_CTX mat3 operator+(const mat3& m) const { return mat3(cols[0]+m[0], cols[1]+m[1], cols[2]+m[2]); }
    SUMI_CTX mat3 operator-(const mat3& m) const { return mat3(cols[0]-m[0], cols[1]-m[1], cols[2]-m[2]); }
    SUMI_CTX mat3 operator*(float s) const { return mat3(cols[0]*s, cols[1]*s, cols[2]*s); }
    SUMI_CTX mat3 operator/(float s) const { return mat3(cols[0]/s, cols[1]/s, cols[2]/s); }
    SUMI_CTX mat3& operator+=(const mat3& m) { cols[0]+=m[0]; cols[1]+=m[1]; cols[2]+=m[2]; return *this; }
    SUMI_CTX mat3& operator-=(const mat3& m) { cols[0]-=m[0]; cols[1]-=m[1]; cols[2]-=m[2]; return *this; }
    SUMI_CTX mat3& operator*=(float s) { cols[0]*=s; cols[1]*=s; cols[2]*=s; return *this; }
    SUMI_CTX mat3& operator/=(float s) { cols[0]/=s; cols[1]/=s; cols[2]/=s; return *this; }
    SUMI_CTX bool operator==(const mat3& m) const { return cols[0] == m[0] && cols[1] == m[1] && cols[2] == m[2]; }
    SUMI_CTX bool operator!=(const mat3& m) const { return !(*this == m); }

    SUMI_CTX mat3 operator*(const mat3& m) const {
        mat3 res(0.0f);
        for(int i=0; i<3; ++i) {
            for(int j=0; j<3; ++j) {
                res[i][j] = cols[0][j]*m[i][0] + cols[1][j]*m[i][1] + cols[2][j]*m[i][2];
            }
        }
        return res;
    }
    SUMI_CTX vec3 operator*(const vec3& v) const {
        return vec3(cols[0]*v.x + cols[1]*v.y + cols[2]*v.z);
    }
};

struct mat4 {
    vec4 cols[4];
    SUMI_CTX mat4() {
        cols[0]=vec4(1,0,0,0); cols[1]=vec4(0,1,0,0); cols[2]=vec4(0,0,1,0); cols[3]=vec4(0,0,0,1);
    }
    SUMI_CTX explicit mat4(float s) {
        cols[0]=vec4(s,0,0,0); cols[1]=vec4(0,s,0,0); cols[2]=vec4(0,0,s,0); cols[3]=vec4(0,0,0,s);
    }
    SUMI_CTX mat4(const vec4& c0, const vec4& c1, const vec4& c2, const vec4& c3) {
        cols[0]=c0; cols[1]=c1; cols[2]=c2; cols[3]=c3;
    }
    SUMI_CTX mat4(float x0, float y0, float z0, float w0,
                  float x1, float y1, float z1, float w1,
                  float x2, float y2, float z2, float w2,
                  float x3, float y3, float z3, float w3) {
        cols[0] = vec4(x0,y0,z0,w0); cols[1] = vec4(x1,y1,z1,w1);
        cols[2] = vec4(x2,y2,z2,w2); cols[3] = vec4(x3,y3,z3,w3);
    }
    
    SUMI_CTX vec4& operator[](int i) { return cols[i]; }
    SUMI_CTX const vec4& operator[](int i) const { return cols[i]; }

    SUMI_CTX mat4 operator+(const mat4& m) const { return mat4(cols[0]+m[0], cols[1]+m[1], cols[2]+m[2], cols[3]+m[3]); }
    SUMI_CTX mat4 operator-(const mat4& m) const { return mat4(cols[0]-m[0], cols[1]-m[1], cols[2]-m[2], cols[3]-m[3]); }
    SUMI_CTX mat4 operator*(float s) const { return mat4(cols[0]*s, cols[1]*s, cols[2]*s, cols[3]*s); }
    SUMI_CTX mat4 operator/(float s) const { return mat4(cols[0]/s, cols[1]/s, cols[2]/s, cols[3]/s); }
    SUMI_CTX mat4& operator+=(const mat4& m) { cols[0]+=m[0]; cols[1]+=m[1]; cols[2]+=m[2]; cols[3]+=m[3]; return *this; }
    SUMI_CTX mat4& operator-=(const mat4& m) { cols[0]-=m[0]; cols[1]-=m[1]; cols[2]-=m[2]; cols[3]-=m[3]; return *this; }
    SUMI_CTX mat4& operator*=(float s) { cols[0]*=s; cols[1]*=s; cols[2]*=s; cols[3]*=s; return *this; }
    SUMI_CTX mat4& operator/=(float s) { cols[0]/=s; cols[1]/=s; cols[2]/=s; cols[3]/=s; return *this; }
    SUMI_CTX bool operator==(const mat4& m) const { return cols[0] == m[0] && cols[1] == m[1] && cols[2] == m[2] && cols[3] == m[3]; }
    SUMI_CTX bool operator!=(const mat4& m) const { return !(*this == m); }

    SUMI_CTX mat4 operator*(const mat4& m) const {
        mat4 res(0.0f);
        for(int i=0; i<4; ++i) { // Column of result
            for(int j=0; j<4; ++j) { // Row of result
                res[i][j] = cols[0][j]*m[i][0] + cols[1][j]*m[i][1] + 
                            cols[2][j]*m[i][2] + cols[3][j]*m[i][3];
            }
        }
        return res;
    }
    SUMI_CTX vec4 operator*(const vec4& v) const {
        return vec4(cols[0]*v.x + cols[1]*v.y + cols[2]*v.z + cols[3]*v.w);
    }
};

inline SUMI_CTX mat2 operator*(float s, const mat2& m) { return m * s; }
inline SUMI_CTX mat3 operator*(float s, const mat3& m) { return m * s; }
inline SUMI_CTX mat4 operator*(float s, const mat4& m) { return m * s; }

struct quat {
    union { float w, r; };
    union { float x, i; };
    union { float y, j; };
    union { float z, k; };

    SUMI_CTX quat() : w(1.0f), x(0.0f), y(0.0f), z(0.0f) {}
    SUMI_CTX explicit quat(float s) : w(s), x(0.0f), y(0.0f), z(0.0f) {}
    SUMI_CTX quat(float _w, float _x, float _y, float _z) : w(_w), x(_x), y(_y), z(_z) {}
    SUMI_CTX quat(float _w, const vec3& v) : w(_w), x(v.x), y(v.y), z(v.z) {}

    SUMI_CTX float& operator[](int idx) { return (&w)[idx]; }
    SUMI_CTX const float& operator[](int idx) const { return (&w)[idx]; }

    SUMI_CTX quat operator-() const { return quat(-w, -x, -y, -z); }
    SUMI_CTX quat operator+(const quat& q) const { return quat(w+q.w, x+q.x, y+q.y, z+q.z); }
    SUMI_CTX quat operator-(const quat& q) const { return quat(w-q.w, x-q.x, y-q.y, z-q.z); }
    SUMI_CTX quat operator*(float s) const { return quat(w*s, x*s, y*s, z*s); }
    SUMI_CTX quat operator/(float s) const { return quat(w/s, x/s, y/s, z/s); }
    SUMI_CTX quat& operator+=(const quat& q) { w+=q.w; x+=q.x; y+=q.y; z+=q.z; return *this; }
    SUMI_CTX quat& operator-=(const quat& q) { w-=q.w; x-=q.x; y-=q.y; z-=q.z; return *this; }
    SUMI_CTX quat& operator*=(float s) { w*=s; x*=s; y*=s; z*=s; return *this; }
    SUMI_CTX quat& operator/=(float s) { w/=s; x/=s; y/=s; z/=s; return *this; }
    SUMI_CTX bool operator==(const quat& q) const { return w == q.w && x == q.x && y == q.y && z == q.z; }
    SUMI_CTX bool operator!=(const quat& q) const { return !(*this == q); }

    SUMI_CTX quat operator*(const quat& q) const {
        return quat(
            w*q.w - x*q.x - y*q.y - z*q.z,
            w*q.x + x*q.w + y*q.z - z*q.y,
            w*q.y - x*q.z + y*q.w + z*q.x,
            w*q.z + x*q.y - y*q.x + z*q.w
        );
    }
    SUMI_CTX quat& operator*=(const quat& q) { *this = *this * q; return *this; }
};

inline SUMI_CTX quat operator*(float s, const quat& q) { return q * s; }

#endif // !SUMI_USE_NATIVE_TYPES

// =================================================================================================
// Shared Utilities & Texture Support
// =================================================================================================

struct Sampler2D {
    int w, h;
    vec4* data;
};

// =================================================================================================
// Standard Math Functions
// =================================================================================================
// If using Native Types (Metal), these math functions are provided by the platform (metal_stdlib)
// via the `using` declarations in sumi_platform.h.
// We only define these for the custom CPU/CUDA structs.

#if !SUMI_USE_NATIVE_TYPES

// Scalar Wrappers
SUMI_CTX inline float radians(float d) { return d * (SUMI_PI / 180.0f); }
SUMI_CTX inline float degrees(float r) { return r * (180.0f / SUMI_PI); }
SUMI_CTX inline float min(float a, float b) { return (a < b) ? a : b; }
SUMI_CTX inline float max(float a, float b) { return (a > b) ? a : b; }
SUMI_CTX inline float clamp(float x, float minVal, float maxVal) { return min(max(x, minVal), maxVal); }
SUMI_CTX inline float mix(float x, float y, float a) { return x * (1.0f - a) + y * a; }
SUMI_CTX inline float step(float edge, float x) { return x < edge ? 0.0f : 1.0f; }
SUMI_CTX inline float smoothstep(float edge0, float edge1, float x) {
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
SUMI_CTX inline float floor(float x) { return ::floorf(x); }
SUMI_CTX inline float trunc(float x) { return ::truncf(x); }
SUMI_CTX inline float round(float x) { return ::roundf(x); }
SUMI_CTX inline float ceil(float x) { return ::ceilf(x); }
SUMI_CTX inline float fract(float x) { return x - floor(x); }
SUMI_CTX inline float mod(float x, float y) { return x - y * floor(x / y); }
SUMI_CTX inline float fma(float a, float b, float c) { return ::fmaf(a, b, c); }
SUMI_CTX inline float abs(float x) { return ::fabsf(x); }
SUMI_CTX inline float sign(float x) { return (x > 0.0f) ? 1.0f : ((x < 0.0f) ? -1.0f : 0.0f); }
SUMI_CTX inline float sin(float x) { return ::sinf(x); }
SUMI_CTX inline float cos(float x) { return ::cosf(x); }
SUMI_CTX inline float tan(float x) { return ::tanf(x); }
SUMI_CTX inline float asin(float x) { return ::asinf(x); }
SUMI_CTX inline float acos(float x) { return ::acosf(x); }
SUMI_CTX inline float atan(float x) { return ::atanf(x); }
SUMI_CTX inline float atan(float y, float x) { return ::atan2f(y, x); }
SUMI_CTX inline float sqrt(float x) { return ::sqrtf(x); }
SUMI_CTX inline float inversesqrt(float x) { return 1.0f / sqrt(x); }
SUMI_CTX inline float exp(float x) { return ::expf(x); }
SUMI_CTX inline float exp2(float x) { return ::exp2f(x); }
SUMI_CTX inline float pow(float x, float y) { return ::powf(x, y); }
SUMI_CTX inline float log(float x) { return ::logf(x); }
SUMI_CTX inline float log2(float x) { return ::log2f(x); }

// Vector Functions (Generics)
#define DEFINE_VEC_OP1(func_name, op_call) \
    SUMI_CTX inline vec2 func_name(const vec2& v) { return vec2(op_call(v.x), op_call(v.y)); } \
    SUMI_CTX inline vec3 func_name(const vec3& v) { return vec3(op_call(v.x), op_call(v.y), op_call(v.z)); } \
    SUMI_CTX inline vec4 func_name(const vec4& v) { return vec4(op_call(v.x), op_call(v.y), op_call(v.z), op_call(v.w)); }

#define DEFINE_VEC_OP2(func_name, op_call) \
    SUMI_CTX inline vec2 func_name(const vec2& a, const vec2& b) { return vec2(op_call(a.x,b.x), op_call(a.y,b.y)); } \
    SUMI_CTX inline vec3 func_name(const vec3& a, const vec3& b) { return vec3(op_call(a.x,b.x), op_call(a.y,b.y), op_call(a.z,b.z)); } \
    SUMI_CTX inline vec4 func_name(const vec4& a, const vec4& b) { return vec4(op_call(a.x,b.x), op_call(a.y,b.y), op_call(a.z,b.z), op_call(a.w,b.w)); }

#define DEFINE_VEC_OP2_SCALAR(func_name, op_call) \
    SUMI_CTX inline vec2 func_name(const vec2& a, float b) { return vec2(op_call(a.x,b), op_call(a.y,b)); } \
    SUMI_CTX inline vec3 func_name(const vec3& a, float b) { return vec3(op_call(a.x,b), op_call(a.y,b), op_call(a.z,b)); } \
    SUMI_CTX inline vec4 func_name(const vec4& a, float b) { return vec4(op_call(a.x,b), op_call(a.y,b), op_call(a.z,b), op_call(a.w,b)); }

DEFINE_VEC_OP1(floor, floor)
DEFINE_VEC_OP1(trunc, trunc)
DEFINE_VEC_OP1(round, round)
DEFINE_VEC_OP1(ceil, ceil)
DEFINE_VEC_OP1(fract, fract)
DEFINE_VEC_OP1(abs, abs)
DEFINE_VEC_OP1(sign, sign)
DEFINE_VEC_OP1(sin, sin)
DEFINE_VEC_OP1(cos, cos)
DEFINE_VEC_OP1(tan, tan)
DEFINE_VEC_OP1(asin, asin)
DEFINE_VEC_OP1(acos, acos)
DEFINE_VEC_OP1(atan, atan)
DEFINE_VEC_OP1(sqrt, sqrt)
DEFINE_VEC_OP1(inversesqrt, inversesqrt)
DEFINE_VEC_OP1(exp, exp)
DEFINE_VEC_OP1(exp2, exp2)
DEFINE_VEC_OP1(log, log)
DEFINE_VEC_OP1(log2, log2)

DEFINE_VEC_OP2(min, min)
DEFINE_VEC_OP2(max, max)
DEFINE_VEC_OP2(mod, mod)
DEFINE_VEC_OP2(pow, pow)
DEFINE_VEC_OP2(atan, atan)

DEFINE_VEC_OP2_SCALAR(min, min)
DEFINE_VEC_OP2_SCALAR(max, max)
DEFINE_VEC_OP2_SCALAR(mod, mod)
DEFINE_VEC_OP2_SCALAR(pow, pow)

// Clamp specializations
SUMI_CTX inline vec2 clamp(const vec2& v, float minVal, float maxVal) { return vec2(clamp(v.x, minVal, maxVal), clamp(v.y, minVal, maxVal)); }
SUMI_CTX inline vec3 clamp(const vec3& v, float minVal, float maxVal) { return vec3(clamp(v.x, minVal, maxVal), clamp(v.y, minVal, maxVal), clamp(v.z, minVal, maxVal)); }
SUMI_CTX inline vec4 clamp(const vec4& v, float minVal, float maxVal) { return vec4(clamp(v.x, minVal, maxVal), clamp(v.y, minVal, maxVal), clamp(v.z, minVal, maxVal), clamp(v.w, minVal, maxVal)); }
SUMI_CTX inline vec2 clamp(const vec2& v, const vec2& minVal, const vec2& maxVal) { return vec2(clamp(v.x, minVal.x, maxVal.x), clamp(v.y, minVal.y, maxVal.y)); }
SUMI_CTX inline vec3 clamp(const vec3& v, const vec3& minVal, const vec3& maxVal) { return vec3(clamp(v.x, minVal.x, maxVal.x), clamp(v.y, minVal.y, maxVal.y), clamp(v.z, minVal.z, maxVal.z)); }
SUMI_CTX inline vec4 clamp(const vec4& v, const vec4& minVal, const vec4& maxVal) { return vec4(clamp(v.x, minVal.x, maxVal.x), clamp(v.y, minVal.y, maxVal.y), clamp(v.z, minVal.z, maxVal.z), clamp(v.w, minVal.w, maxVal.w)); }

// Mix specializations
SUMI_CTX inline vec2 mix(const vec2& x, const vec2& y, float a) { return x * (1.0f - a) + y * a; }
SUMI_CTX inline vec3 mix(const vec3& x, const vec3& y, float a) { return x * (1.0f - a) + y * a; }
SUMI_CTX inline vec4 mix(const vec4& x, const vec4& y, float a) { return x * (1.0f - a) + y * a; }
SUMI_CTX inline vec2 mix(const vec2& x, const vec2& y, const vec2& a) { return x * (vec2(1.0f) - a) + y * a; }
SUMI_CTX inline vec3 mix(const vec3& x, const vec3& y, const vec3& a) { return x * (vec3(1.0f) - a) + y * a; }
SUMI_CTX inline vec4 mix(const vec4& x, const vec4& y, const vec4& a) { return x * (vec4(1.0f) - a) + y * a; }
SUMI_CTX inline vec2 mix(const vec2& x, const vec2& y, const bvec2& a) { return vec2(a.x ? y.x : x.x, a.y ? y.y : x.y); }
SUMI_CTX inline vec3 mix(const vec3& x, const vec3& y, const bvec3& a) { return vec3(a.x ? y.x : x.x, a.y ? y.y : x.y, a.z ? y.z : x.z); }
SUMI_CTX inline vec4 mix(const vec4& x, const vec4& y, const bvec4& a) { return vec4(a.x ? y.x : x.x, a.y ? y.y : x.y, a.z ? y.z : x.z, a.w ? y.w : x.w); }

// Step and smoothstep specializations
SUMI_CTX inline vec2 step(float edge, const vec2& x) { return vec2(step(edge, x.x), step(edge, x.y)); }
SUMI_CTX inline vec3 step(float edge, const vec3& x) { return vec3(step(edge, x.x), step(edge, x.y), step(edge, x.z)); }
SUMI_CTX inline vec4 step(float edge, const vec4& x) { return vec4(step(edge, x.x), step(edge, x.y), step(edge, x.z), step(edge, x.w)); }
SUMI_CTX inline vec2 step(const vec2& edge, const vec2& x) { return vec2(step(edge.x, x.x), step(edge.y, x.y)); }
SUMI_CTX inline vec3 step(const vec3& edge, const vec3& x) { return vec3(step(edge.x, x.x), step(edge.y, x.y), step(edge.z, x.z)); }
SUMI_CTX inline vec4 step(const vec4& edge, const vec4& x) { return vec4(step(edge.x, x.x), step(edge.y, x.y), step(edge.z, x.z), step(edge.w, x.w)); }

SUMI_CTX inline vec2 smoothstep(float edge0, float edge1, const vec2& x) { return vec2(smoothstep(edge0, edge1, x.x), smoothstep(edge0, edge1, x.y)); }
SUMI_CTX inline vec3 smoothstep(float edge0, float edge1, const vec3& x) { return vec3(smoothstep(edge0, edge1, x.x), smoothstep(edge0, edge1, x.y), smoothstep(edge0, edge1, x.z)); }
SUMI_CTX inline vec4 smoothstep(float edge0, float edge1, const vec4& x) { return vec4(smoothstep(edge0, edge1, x.x), smoothstep(edge0, edge1, x.y), smoothstep(edge0, edge1, x.z), smoothstep(edge0, edge1, x.w)); }
SUMI_CTX inline vec2 smoothstep(const vec2& edge0, const vec2& edge1, const vec2& x) { return vec2(smoothstep(edge0.x, edge1.x, x.x), smoothstep(edge0.y, edge1.y, x.y)); }
SUMI_CTX inline vec3 smoothstep(const vec3& edge0, const vec3& edge1, const vec3& x) { return vec3(smoothstep(edge0.x, edge1.x, x.x), smoothstep(edge0.y, edge1.y, x.y), smoothstep(edge0.z, edge1.z, x.z)); }
SUMI_CTX inline vec4 smoothstep(const vec4& edge0, const vec4& edge1, const vec4& x) { return vec4(smoothstep(edge0.x, edge1.x, x.x), smoothstep(edge0.y, edge1.y, x.y), smoothstep(edge0.z, edge1.z, x.z), smoothstep(edge0.w, edge1.w, x.w)); }

// Floating point fused multiply-add
SUMI_CTX inline vec2 fma(const vec2& a, const vec2& b, const vec2& c) { return vec2(fma(a.x, b.x, c.x), fma(a.y, b.y, c.y)); }
SUMI_CTX inline vec3 fma(const vec3& a, const vec3& b, const vec3& c) { return vec3(fma(a.x, b.x, c.x), fma(a.y, b.y, c.y), fma(a.z, b.z, c.z)); }
SUMI_CTX inline vec4 fma(const vec4& a, const vec4& b, const vec4& c) { return vec4(fma(a.x, b.x, c.x), fma(a.y, b.y, c.y), fma(a.z, b.z, c.z), fma(a.w, b.w, c.w)); }

// Relational functions
SUMI_CTX inline bool all(const bvec2& v) { return v.x && v.y; }
SUMI_CTX inline bool all(const bvec3& v) { return v.x && v.y && v.z; }
SUMI_CTX inline bool all(const bvec4& v) { return v.x && v.y && v.z && v.w; }
SUMI_CTX inline bool any(const bvec2& v) { return v.x || v.y; }
SUMI_CTX inline bool any(const bvec3& v) { return v.x || v.y || v.z; }
SUMI_CTX inline bool any(const bvec4& v) { return v.x || v.y || v.z || v.w; }
SUMI_CTX inline bvec2 not_(const bvec2& v) { return bvec2(!v.x, !v.y); }
SUMI_CTX inline bvec3 not_(const bvec3& v) { return bvec3(!v.x, !v.y, !v.z); }
SUMI_CTX inline bvec4 not_(const bvec4& v) { return bvec4(!v.x, !v.y, !v.z, !v.w); }
SUMI_CTX inline bvec2 equal(const vec2& a, const vec2& b) { return bvec2(a.x == b.x, a.y == b.y); }
SUMI_CTX inline bvec3 equal(const vec3& a, const vec3& b) { return bvec3(a.x == b.x, a.y == b.y, a.z == b.z); }
SUMI_CTX inline bvec4 equal(const vec4& a, const vec4& b) { return bvec4(a.x == b.x, a.y == b.y, a.z == b.z, a.w == b.w); }
SUMI_CTX inline bvec2 notEqual(const vec2& a, const vec2& b) { return bvec2(a.x != b.x, a.y != b.y); }
SUMI_CTX inline bvec3 notEqual(const vec3& a, const vec3& b) { return bvec3(a.x != b.x, a.y != b.y, a.z != b.z); }
SUMI_CTX inline bvec4 notEqual(const vec4& a, const vec4& b) { return bvec4(a.x != b.x, a.y != b.y, a.z != b.z, a.w != b.w); }
SUMI_CTX inline bvec2 lessThan(const vec2& a, const vec2& b) { return bvec2(a.x < b.x, a.y < b.y); }
SUMI_CTX inline bvec3 lessThan(const vec3& a, const vec3& b) { return bvec3(a.x < b.x, a.y < b.y, a.z < b.z); }
SUMI_CTX inline bvec4 lessThan(const vec4& a, const vec4& b) { return bvec4(a.x < b.x, a.y < b.y, a.z < b.z, a.w < b.w); }
SUMI_CTX inline bvec2 lessThanEqual(const vec2& a, const vec2& b) { return bvec2(a.x <= b.x, a.y <= b.y); }
SUMI_CTX inline bvec3 lessThanEqual(const vec3& a, const vec3& b) { return bvec3(a.x <= b.x, a.y <= b.y, a.z <= b.z); }
SUMI_CTX inline bvec4 lessThanEqual(const vec4& a, const vec4& b) { return bvec4(a.x <= b.x, a.y <= b.y, a.z <= b.z, a.w <= b.w); }
SUMI_CTX inline bvec2 greaterThan(const vec2& a, const vec2& b) { return bvec2(a.x > b.x, a.y > b.y); }
SUMI_CTX inline bvec3 greaterThan(const vec3& a, const vec3& b) { return bvec3(a.x > b.x, a.y > b.y, a.z > b.z); }
SUMI_CTX inline bvec4 greaterThan(const vec4& a, const vec4& b) { return bvec4(a.x > b.x, a.y > b.y, a.z > b.z, a.w > b.w); }
SUMI_CTX inline bvec2 greaterThanEqual(const vec2& a, const vec2& b) { return bvec2(a.x >= b.x, a.y >= b.y); }
SUMI_CTX inline bvec3 greaterThanEqual(const vec3& a, const vec3& b) { return bvec3(a.x >= b.x, a.y >= b.y, a.z >= b.z); }
SUMI_CTX inline bvec4 greaterThanEqual(const vec4& a, const vec4& b) { return bvec4(a.x >= b.x, a.y >= b.y, a.z >= b.z, a.w >= b.w); }

// Geometric Functions
SUMI_CTX inline float dot(const vec2& a, const vec2& b) { return a.x*b.x + a.y*b.y; }
SUMI_CTX inline float dot(const vec3& a, const vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
SUMI_CTX inline float dot(const vec4& a, const vec4& b) { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }

SUMI_CTX inline vec3 cross(const vec3& a, const vec3& b) {
    return vec3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
}

SUMI_CTX inline float length(const vec2& v) { return sqrt(dot(v, v)); }
SUMI_CTX inline float length(const vec3& v) { return sqrt(dot(v, v)); }
SUMI_CTX inline float length(const vec4& v) { return sqrt(dot(v, v)); }

SUMI_CTX inline float distance(const vec2& a, const vec2& b) { return length(a - b); }
SUMI_CTX inline float distance(const vec3& a, const vec3& b) { return length(a - b); }
SUMI_CTX inline float distance(const vec4& a, const vec4& b) { return length(a - b); }

SUMI_CTX inline vec2 normalize(const vec2& v) { float l = length(v); return l > SUMI_EPSILON ? v / l : vec2(0.0f); }
SUMI_CTX inline vec3 normalize(const vec3& v) { float l = length(v); return l > SUMI_EPSILON ? v / l : vec3(0.0f); }
SUMI_CTX inline vec4 normalize(const vec4& v) { float l = length(v); return l > SUMI_EPSILON ? v / l : vec4(0.0f); }

SUMI_CTX inline vec2 faceforward(const vec2& N, const vec2& I, const vec2& Nref) {
    return dot(Nref, I) < 0.0f ? N : -N;
}

SUMI_CTX inline vec3 faceforward(const vec3& N, const vec3& I, const vec3& Nref) {
    return dot(Nref, I) < 0.0f ? N : -N;
}

SUMI_CTX inline vec4 faceforward(const vec4& N, const vec4& I, const vec4& Nref) {
    return dot(Nref, I) < 0.0f ? N : -N;
}

SUMI_CTX inline vec2 reflect(const vec2& I, const vec2& N) {
    return I - 2.0f * dot(N, I) * N;
}

SUMI_CTX inline vec3 reflect(const vec3& I, const vec3& N) {
    return I - 2.0f * dot(N, I) * N;
}

SUMI_CTX inline vec4 reflect(const vec4& I, const vec4& N) {
    return I - 2.0f * dot(N, I) * N;
}

SUMI_CTX inline vec2 refract(const vec2& I, const vec2& N, float eta) {
    float d = dot(N, I);
    float k = 1.0f - eta * eta * (1.0f - d * d);
    if (k < 0.0f) return vec2(0.0f);
    else return eta * I - (eta * d + sqrt(k)) * N;
}

SUMI_CTX inline vec3 refract(const vec3& I, const vec3& N, float eta) {
    float d = dot(N, I);
    float k = 1.0f - eta * eta * (1.0f - d * d);
    if (k < 0.0f) return vec3(0.0f);
    else return eta * I - (eta * d + sqrt(k)) * N;
}

SUMI_CTX inline vec4 refract(const vec4& I, const vec4& N, float eta) {
    float d = dot(N, I);
    float k = 1.0f - eta * eta * (1.0f - d * d);
    if (k < 0.0f) return vec4(0.0f);
    else return eta * I - (eta * d + sqrt(k)) * N;
}

// Matrix Transforms
SUMI_CTX inline mat4 translate(const mat4& m, const vec3& v) {
    mat4 r = m;
    r.cols[3] = m.cols[0]*v.x + m.cols[1]*v.y + m.cols[2]*v.z + m.cols[3];
    return r;
}

SUMI_CTX inline mat4 translate(const vec3& v) {
    return translate(mat4(1.0f), v);
}

SUMI_CTX inline mat4 scale(const mat4& m, const vec3& v) {
    mat4 r;
    r.cols[0] = m.cols[0] * v.x;
    r.cols[1] = m.cols[1] * v.y;
    r.cols[2] = m.cols[2] * v.z;
    r.cols[3] = m.cols[3];
    return r;
}

SUMI_CTX inline mat4 scale(const vec3& v) {
    return scale(mat4(1.0f), v);
}

SUMI_CTX inline mat4 rotate(const mat4& m, float angle, const vec3& axis) {
    float c = cos(angle);
    float s = sin(angle);
    vec3 a = normalize(axis);
    vec3 temp = a * (1.0f - c);

    mat4 R; // Identity by default
    R[0][0] = c + temp.x * a.x;
    R[0][1] = temp.x * a.y + s * a.z;
    R[0][2] = temp.x * a.z - s * a.y;

    R[1][0] = temp.y * a.x - s * a.z;
    R[1][1] = c + temp.y * a.y;
    R[1][2] = temp.y * a.z + s * a.x;

    R[2][0] = temp.z * a.x + s * a.y;
    R[2][1] = temp.z * a.y - s * a.x;
    R[2][2] = c + temp.z * a.z;

    // Last row/col remains identity for rotation matrix R
    // R[3][3] is 1.0 from constructor
    return m * R;
}

SUMI_CTX inline mat4 rotate(float angle, const vec3& axis) {
    return rotate(mat4(1.0f), angle, axis);
}

SUMI_CTX inline mat4 perspective(float fovy, float aspect, float zNear, float zFar) {
    float tanHalfFovy = tan(fovy / 2.0f);
    mat4 r(0.0f);
    r[0][0] = 1.0f / (aspect * tanHalfFovy);
    r[1][1] = 1.0f / (tanHalfFovy);
    r[2][2] = -(zFar + zNear) / (zFar - zNear);
    r[2][3] = -1.0f;
    r[3][2] = -(2.0f * zFar * zNear) / (zFar - zNear);
    return r;
}

SUMI_CTX inline mat4 perspectiveRH(float fovy, float aspect, float zNear, float zFar) {
    return perspective(fovy, aspect, zNear, zFar);
}

SUMI_CTX inline mat4 frustum(float left, float right, float bottom, float top, float zNear, float zFar) {
    mat4 r(0.0f);
    r[0][0] = (2.0f * zNear) / (right - left);
    r[1][1] = (2.0f * zNear) / (top - bottom);
    r[2][0] = (right + left) / (right - left);
    r[2][1] = (top + bottom) / (top - bottom);
    r[2][2] = -(zFar + zNear) / (zFar - zNear);
    r[2][3] = -1.0f;
    r[3][2] = -(2.0f * zFar * zNear) / (zFar - zNear);
    return r;
}

SUMI_CTX inline mat4 ortho(float left, float right, float bottom, float top) {
    mat4 r(1.0f);
    r[0][0] = 2.0f / (right - left);
    r[1][1] = 2.0f / (top - bottom);
    r[3][0] = -(right + left) / (right - left);
    r[3][1] = -(top + bottom) / (top - bottom);
    return r;
}

SUMI_CTX inline mat4 ortho(float left, float right, float bottom, float top, float zNear, float zFar) {
    mat4 r(1.0f);
    r[0][0] = 2.0f / (right - left);
    r[1][1] = 2.0f / (top - bottom);
    r[2][2] = -2.0f / (zFar - zNear);
    r[3][0] = -(right + left) / (right - left);
    r[3][1] = -(top + bottom) / (top - bottom);
    r[3][2] = -(zFar + zNear) / (zFar - zNear);
    return r;
}

SUMI_CTX inline mat4 lookAt(const vec3& eye, const vec3& center, const vec3& up) {
    vec3 f(normalize(center - eye));
    vec3 s(normalize(cross(f, up)));
    vec3 u(cross(s, f));

    mat4 r(1.0f);
    r[0][0] = s.x; r[1][0] = s.y; r[2][0] = s.z;
    r[0][1] = u.x; r[1][1] = u.y; r[2][1] = u.z;
    r[0][2] =-f.x; r[1][2] =-f.y; r[2][2] =-f.z;
    r[3][0] =-dot(s, eye);
    r[3][1] =-dot(u, eye);
    r[3][2] = dot(f, eye);
    return r;
}

SUMI_CTX inline mat4 lookAtRH(const vec3& eye, const vec3& center, const vec3& up) {
    return lookAt(eye, center, up);
}

SUMI_CTX inline mat4 lookAtLH(const vec3& eye, const vec3& center, const vec3& up) {
    vec3 f(normalize(center - eye));
    vec3 s(normalize(cross(up, f)));
    vec3 u(cross(f, s));

    mat4 r(1.0f);
    r[0][0] = s.x; r[1][0] = s.y; r[2][0] = s.z;
    r[0][1] = u.x; r[1][1] = u.y; r[2][1] = u.z;
    r[0][2] = f.x; r[1][2] = f.y; r[2][2] = f.z;
    r[3][0] = -dot(s, eye);
    r[3][1] = -dot(u, eye);
    r[3][2] = -dot(f, eye);
    return r;
}

// Matrix Operations
SUMI_CTX inline mat2 transpose(const mat2& m) {
    return mat2(
        m[0][0], m[1][0],
        m[0][1], m[1][1]
    );
}

SUMI_CTX inline mat3 transpose(const mat3& m) {
    return mat3(
        m[0][0], m[1][0], m[2][0],
        m[0][1], m[1][1], m[2][1],
        m[0][2], m[1][2], m[2][2]
    );
}

SUMI_CTX inline mat4 transpose(const mat4& m) {
    return mat4(
        m[0][0], m[1][0], m[2][0], m[3][0],
        m[0][1], m[1][1], m[2][1], m[3][1],
        m[0][2], m[1][2], m[2][2], m[3][2],
        m[0][3], m[1][3], m[2][3], m[3][3]
    );
}

SUMI_CTX inline mat2 matrixCompMult(const mat2& a, const mat2& b) {
    return mat2(a[0] * b[0], a[1] * b[1]);
}

SUMI_CTX inline mat3 matrixCompMult(const mat3& a, const mat3& b) {
    return mat3(a[0] * b[0], a[1] * b[1], a[2] * b[2]);
}

SUMI_CTX inline mat4 matrixCompMult(const mat4& a, const mat4& b) {
    return mat4(a[0] * b[0], a[1] * b[1], a[2] * b[2], a[3] * b[3]);
}

SUMI_CTX inline mat2 outerProduct(const vec2& c, const vec2& r) {
    return mat2(c * r.x, c * r.y);
}

SUMI_CTX inline mat3 outerProduct(const vec3& c, const vec3& r) {
    return mat3(c * r.x, c * r.y, c * r.z);
}

SUMI_CTX inline mat4 outerProduct(const vec4& c, const vec4& r) {
    return mat4(c * r.x, c * r.y, c * r.z, c * r.w);
}

SUMI_CTX inline float determinant(const mat2& m) {
    return m[0][0] * m[1][1] - m[1][0] * m[0][1];
}

SUMI_CTX inline float determinant(const mat3& m) {
    float a00 = m[0][0], a01 = m[1][0], a02 = m[2][0];
    float a10 = m[0][1], a11 = m[1][1], a12 = m[2][1];
    float a20 = m[0][2], a21 = m[1][2], a22 = m[2][2];
    return a00 * (a11 * a22 - a12 * a21)
         - a01 * (a10 * a22 - a12 * a20)
         + a02 * (a10 * a21 - a11 * a20);
}

SUMI_CTX inline float determinant(const mat4& m) {
    float a[4][4];
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            a[row][col] = m[col][row];
        }
    }

    float det = 1.0f;
    float sign = 1.0f;
    for (int col = 0; col < 4; ++col) {
        int pivot = col;
        for (int row = col + 1; row < 4; ++row) {
            if (abs(a[row][col]) > abs(a[pivot][col])) {
                pivot = row;
            }
        }
        if (abs(a[pivot][col]) <= SUMI_EPSILON) {
            return 0.0f;
        }
        if (pivot != col) {
            for (int j = 0; j < 4; ++j) {
                float tmp = a[col][j];
                a[col][j] = a[pivot][j];
                a[pivot][j] = tmp;
            }
            sign = -sign;
        }

        float pivotValue = a[col][col];
        det *= pivotValue;
        for (int row = col + 1; row < 4; ++row) {
            float factor = a[row][col] / pivotValue;
            for (int j = col; j < 4; ++j) {
                a[row][j] -= factor * a[col][j];
            }
        }
    }
    return det * sign;
}

SUMI_CTX inline mat2 inverse(const mat2& m) {
    float det = determinant(m);
    if (abs(det) <= SUMI_EPSILON) return mat2(0.0f);
    float invDet = 1.0f / det;
    return mat2(
         m[1][1] * invDet, -m[0][1] * invDet,
        -m[1][0] * invDet,  m[0][0] * invDet
    );
}

SUMI_CTX inline mat3 inverse(const mat3& m) {
    float det = determinant(m);
    if (abs(det) <= SUMI_EPSILON) return mat3(0.0f);
    float invDet = 1.0f / det;
    mat3 r(0.0f);
    r[0][0] =  (m[1][1] * m[2][2] - m[2][1] * m[1][2]) * invDet;
    r[0][1] = -(m[0][1] * m[2][2] - m[2][1] * m[0][2]) * invDet;
    r[0][2] =  (m[0][1] * m[1][2] - m[1][1] * m[0][2]) * invDet;
    r[1][0] = -(m[1][0] * m[2][2] - m[2][0] * m[1][2]) * invDet;
    r[1][1] =  (m[0][0] * m[2][2] - m[2][0] * m[0][2]) * invDet;
    r[1][2] = -(m[0][0] * m[1][2] - m[1][0] * m[0][2]) * invDet;
    r[2][0] =  (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * invDet;
    r[2][1] = -(m[0][0] * m[2][1] - m[2][0] * m[0][1]) * invDet;
    r[2][2] =  (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * invDet;
    return r;
}

SUMI_CTX inline mat4 inverse(const mat4& m) {
    float a[4][8];
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            a[row][col] = m[col][row];
            a[row][col + 4] = row == col ? 1.0f : 0.0f;
        }
    }

    for (int col = 0; col < 4; ++col) {
        int pivot = col;
        for (int row = col + 1; row < 4; ++row) {
            if (abs(a[row][col]) > abs(a[pivot][col])) {
                pivot = row;
            }
        }
        if (abs(a[pivot][col]) <= SUMI_EPSILON) {
            return mat4(0.0f);
        }
        if (pivot != col) {
            for (int j = 0; j < 8; ++j) {
                float tmp = a[col][j];
                a[col][j] = a[pivot][j];
                a[pivot][j] = tmp;
            }
        }

        float invPivot = 1.0f / a[col][col];
        for (int j = 0; j < 8; ++j) {
            a[col][j] *= invPivot;
        }

        for (int row = 0; row < 4; ++row) {
            if (row == col) continue;
            float factor = a[row][col];
            for (int j = 0; j < 8; ++j) {
                a[row][j] -= factor * a[col][j];
            }
        }
    }

    mat4 r(0.0f);
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            r[col][row] = a[row][col + 4];
        }
    }
    return r;
}

// Quaternion Operations
SUMI_CTX inline float dot(const quat& a, const quat& b) {
    return a.w*b.w + a.x*b.x + a.y*b.y + a.z*b.z;
}

SUMI_CTX inline float length(const quat& q) {
    return sqrt(dot(q, q));
}

SUMI_CTX inline quat normalize(const quat& q) {
    float l = length(q);
    return l > SUMI_EPSILON ? q / l : quat();
}

SUMI_CTX inline quat conjugate(const quat& q) {
    return quat(q.w, -q.x, -q.y, -q.z);
}

SUMI_CTX inline quat inverse(const quat& q) {
    float d = dot(q, q);
    return d > SUMI_EPSILON ? conjugate(q) / d : quat();
}

SUMI_CTX inline quat angleAxis(float angle, const vec3& axis) {
    vec3 n = normalize(axis);
    float halfAngle = angle * 0.5f;
    float s = sin(halfAngle);
    return quat(cos(halfAngle), n.x * s, n.y * s, n.z * s);
}

SUMI_CTX inline float angle(const quat& q) {
    quat n = normalize(q);
    return 2.0f * acos(clamp(n.w, -1.0f, 1.0f));
}

SUMI_CTX inline vec3 axis(const quat& q) {
    quat n = normalize(q);
    float s2 = 1.0f - n.w * n.w;
    if (s2 <= SUMI_EPSILON) return vec3(0.0f, 0.0f, 1.0f);
    float invS = inversesqrt(s2);
    return vec3(n.x * invS, n.y * invS, n.z * invS);
}

SUMI_CTX inline mat3 mat3_cast(const quat& q) {
    quat n = normalize(q);
    float xx = n.x * n.x;
    float yy = n.y * n.y;
    float zz = n.z * n.z;
    float xy = n.x * n.y;
    float xz = n.x * n.z;
    float yz = n.y * n.z;
    float wx = n.w * n.x;
    float wy = n.w * n.y;
    float wz = n.w * n.z;

    return mat3(
        1.0f - 2.0f * (yy + zz), 2.0f * (xy + wz),        2.0f * (xz - wy),
        2.0f * (xy - wz),        1.0f - 2.0f * (xx + zz), 2.0f * (yz + wx),
        2.0f * (xz + wy),        2.0f * (yz - wx),        1.0f - 2.0f * (xx + yy)
    );
}

SUMI_CTX inline mat4 mat4_cast(const quat& q) {
    mat3 r = mat3_cast(q);
    return mat4(
        vec4(r[0], 0.0f),
        vec4(r[1], 0.0f),
        vec4(r[2], 0.0f),
        vec4(0.0f, 0.0f, 0.0f, 1.0f)
    );
}

SUMI_CTX inline mat4 toMat4(const quat& q) {
    return mat4_cast(q);
}

SUMI_CTX inline vec3 operator*(const quat& q, const vec3& v) {
    vec3 qv(q.x, q.y, q.z);
    vec3 t = 2.0f * cross(qv, v);
    return v + q.w * t + cross(qv, t);
}

SUMI_CTX inline vec3 rotate(const quat& q, const vec3& v) {
    return q * v;
}

SUMI_CTX inline quat mix(const quat& x, const quat& y, float a) {
    return normalize(x * (1.0f - a) + y * a);
}

SUMI_CTX inline quat slerp(const quat& x, const quat& y, float a) {
    quat z = y;
    float cosTheta = dot(x, y);
    if (cosTheta < 0.0f) {
        z = -y;
        cosTheta = -cosTheta;
    }
    if (cosTheta > 1.0f - SUMI_EPSILON) {
        return mix(x, z, a);
    }

    float theta = acos(clamp(cosTheta, -1.0f, 1.0f));
    float sinTheta = sin(theta);
    float w0 = sin((1.0f - a) * theta) / sinTheta;
    float w1 = sin(a * theta) / sinTheta;
    return normalize(x * w0 + z * w1);
}

// Texture Sampling (Basic CPU Implementation)
// Note: Metal uses texture2d<T> objects, so this function is only for the CPU implementation.
SUMI_CTX inline vec4 texture(const Sampler2D& sampler, const vec2& uv) {
    if (!sampler.data) return vec4(0.0f, 0.0f, 0.0f, 1.0f);
    
    float fx = uv.x - floor(uv.x);
    float fy = uv.y - floor(uv.y);
    if(fx < 0) fx += 1.0f;
    if(fy < 0) fy += 1.0f;

    int tx = (int)(fx * sampler.w);
    int ty = (int)(fy * sampler.h);

    if(tx < 0) tx = 0;
    if(tx >= sampler.w) tx = sampler.w - 1;
    
    if(ty < 0) ty = 0; 
    if(ty >= sampler.h) ty = sampler.h - 1;

    return sampler.data[ty * sampler.w + tx];
}

// IO Operators (Host only)
inline std::ostream& operator<<(std::ostream& os, const vec2& v) {
    os << "(" << v.x << ", " << v.y << ")";
    return os;
}
inline std::ostream& operator<<(std::ostream& os, const vec3& v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}
inline std::ostream& operator<<(std::ostream& os, const vec4& v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
    return os;
}

#endif // !SUMI_USE_NATIVE_TYPES

} // namespace sumi

// Alias for ease of migration/use
namespace glsl = sumi;
