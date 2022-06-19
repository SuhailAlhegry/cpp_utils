#if !defined(ACHILLES_MATH_HPP)
#define ACHILLES_MATH_HPP

// this file depends on <cmath> for 'sqrt'
#include <cmath>
#include "types.hpp"

#define between(c, min, max) ((a) >= (min) && (a) <= (max))

namespace achilles {
    namespace math {
        constexpr f32 TAU        = 6.28318530717958647692f;
        constexpr f32 DEG_TO_RAD = TAU / 360.0f;
        constexpr f32 RAD_TO_DEG = 360.0f / TAU;
        constexpr f32 E          = 2.718281828459f;

        inline f32 fisqrt(f32 n) {
            union {
                f32 f;
                u32 i;
            } conv = { .f = n };
            conv.i = 0x5F1FFFF9ul - ( conv.i >> 1 );
            conv.f *= 0.703952253f * ( 2.38924456f - n * conv.f * conv.f );
            return conv.f;
        }
        
        constexpr inline f32 sign(f32 n) {
            return n >= 0.0f ? 1.0f : -1.0f;
        }

        constexpr inline f32 abs(f32 n) {
            return n < 0.0f ? -n : n;
        }

        constexpr inline f32 max(f32 a, f32 b) {
            return a > b ? a : b;
        }

        constexpr inline f32 min(f32 a, f32 b) {
            return a > b ? b : a;
        }

        constexpr inline f32 lerp(f32 a, f32 b, f32 t) {
            return (1 - t) * a + t * b;
        }

        constexpr inline f32 clamp(f32 a, f32 min, f32 max) {
            if (a > max) return max;
            else if (a < min) return min;
            return a;
        }

        constexpr inline f32 clamp01(f32 a) {
            return clamp(a, 0.0f, 1.0f);
        }

        constexpr inline u64 clampi(u64 a, u64 min, u64 max) {
            if (a > max) return max;
            else if (a < min) return min;
            return a;
        }

        constexpr inline u64 clamp01(u64 a) {
            return clamp(a, 0.0f, 1.0f);
        }

        constexpr inline f32 lerpClamped(f32 a, f32 b, f32 t) {
            t = clamp01(t);
            return (1 - t) * a + t * b;
        }

        constexpr inline f32 inverseLerp(f32 a, f32 b, f32 c) {
            if (a == b) return 0.0f;
            return (c - a) / (b - a);
        }

        constexpr inline f32 inverseLerpClamped(f32 a, f32 b, f32 c) {
            if (a == b) return 0.0f;
            return clamp01((c - a) / (b - a));
        }

        constexpr inline f32 remap(f32 inMin, f32 inMax, f32 outMin, f32 outMax, f32 c) {
            f32 t = inverseLerp(inMin, inMax, c);
            return lerp(outMin, outMax, t);
        }

        constexpr inline f32 remapClamped(f32 inMin, f32 inMax, f32 outMin, f32 outMax, f32 c) {
            f32 t = inverseLerpClamped(inMin, inMax, c);
            return lerp(outMin, outMax, t);
        }

        union float2;
        union float3;
        union float4;
        union quaternion;
        union float4x4;

        union float2 {
            struct {
                f32 x,y;
            };

            struct {
                f32 values[2];
            };

            constexpr float2(f32 x = 0.0f, f32 y = 0.0f) : x(x), y(y) {}
            
            constexpr bool operator==(float2 other) const {
                return this->x == other.x && this->y == other.y;
            }
            
            constexpr bool operator!=(float2 other) const {
                return this->x != other.x || this->y != other.y;
            }
            
            constexpr f32 operator [](u8 index) {
                index = clampi(index, 0, 1);
                return values[index];
            }

            constexpr operator float3() const;
            constexpr operator float4() const;
            constexpr float2 operator-();
            constexpr float2 operator +(float2 b) const;
            constexpr float2 & operator +=(float2 b);
            constexpr float2 operator -(float2 b) const;
            constexpr float2 & operator -=(float2 b);
            constexpr float2 operator /(f32 scalar) const;
            constexpr float2 & operator /=(f32 scalar);
            constexpr float2 operator *(f32 scalar) const;
            constexpr float2 & operator *=(f32 scalar);
            constexpr f32 sqrMagnitude() const;
            f32 magnitude() const;
            float2 & normalize();
            float2 normalized() const;
            constexpr f32 dot(float2 v) const;
            constexpr f32 det(float2 v) const;
            constexpr static f32 dot(float2 a, float2 b);
            constexpr static f32 det(float2 a, float2 b);
            constexpr static float2 one();
            constexpr static float2 up();
            constexpr static float2 right();
            constexpr static float2 down();
            constexpr static float2 left();
            constexpr static float2 lerp(float2 a, float2 b, f32 t);
            constexpr static f32 inverseLerp(float2 a, float2 b, float2 c);
        };

        union float3 {
            struct {
                f32 x,y,z;
            };
            struct {
                f32 r,g,b;
            };
            f32 values[3];

            constexpr float3(f32 x = 0.0f, f32 y = 0.0f, f32 z = 0.0f) : x(x), y(y), z(z) {}
            
            constexpr bool operator==(float3 other) const {
                return this->x == other.x && this->y == other.y && this->z == other.z;
            }
            
            constexpr bool operator!=(float3 other) const {
                return this->x != other.x || this->y != other.y || this->z != other.z;
            }
            
            constexpr f32 operator [](u8 index) {
                index = clampi(index, 0, 2);
                return values[index];
            }

            constexpr explicit operator float2() const;
            constexpr operator float4() const;
            constexpr float3 operator-() const;
            constexpr float3 operator +(float3 b) const;
            constexpr float3 & operator +=(float3 b);
            constexpr float3 operator -(float3 b) const;
            constexpr float3 & operator -=(float3 b);
            constexpr float3 operator /(f32 scalar) const;
            constexpr float3 & operator /=(f32 scalar);
            constexpr float3 operator *(f32 scalar) const;
            constexpr float3 & operator *=(f32 scalar);
            constexpr f32 dot(float3 v) const;
            constexpr float3 cross(float3 v) const;
            constexpr f32 sqrMagnitude() const;
            f32 magnitude() const;
            float3 & normalize();
            float3 normalized() const;
            constexpr static f32 dot(float3 a, float3 b);
            constexpr static float3 cross(float3 a, float3 b);
            constexpr static float3 one();
            constexpr static float3 up();
            constexpr static float3 right();
            constexpr static float3 down();
            constexpr static float3 left();
            constexpr static float3 forward();
            constexpr static float3 back();
            constexpr static float3 lerp(float3 a, float3 b, f32 t);
            constexpr static f32 inverseLerp(float3 a, float3 b, float3 c);
        };

        union float4 {
            struct {
                f32 x,y,z,w;
            };
            
            struct {
                f32 r,g,b,a;
            };
            
            f32 values[4];

            constexpr float4(f32 x = 0.0f, f32 y = 0.0f, f32 z = 0.0f, f32 w = 0.0f) : x(x), y(y), z(z), w(w) {}
            
            constexpr bool operator==(float4 other) const {
                return this->x == other.x && this->y == other.y && this->z == other.z && this->w == other.w;
            }
            
            constexpr bool operator!=(float4 other) const {
                return this->x != other.x || this->y != other.y || this->z != other.z || this->w != other.w;
            }

            constexpr f32 operator [](u8 index) {
                index = clampi(index, 0, 3);
                return values[index];
            }
            
            constexpr explicit operator float2() const;
            constexpr explicit operator float3() const;
            constexpr float4 operator -() const;
            constexpr float4 operator +(float4 b) const;
            constexpr float4 & operator +=(float4 b);
            constexpr float4 operator -(float4 b) const;
            constexpr float4 & operator -=(float4 b);
            constexpr float4 operator /(f32 scalar) const;
            constexpr float4 & operator /=(f32 scalar);
            constexpr float4 operator *(f32 scalar) const;
            constexpr float4 & operator *=(f32 scalar);
            constexpr f32 dot(float4 v) const;
            constexpr f32 sqrMagnitude() const;
            f32 magnitude() const;
            float4 & normalize();
            float4 normalized() const;
            constexpr static f32 dot(float4 a, float4 b);
            constexpr static float4 lerp(float4 a, float4 b, f32 t);
            constexpr static f32 inverseLerp(float4 a, float4 b, float4 c);
        };

        union quaternion {
            struct {
                f32 x, y, z, w;
            };

            struct {
                f32 x, y, z;
            } vector;

            f32 scalar;

            constexpr quaternion(
                f32 x = 0.0f,
                f32 y = 0.0f,
                f32 z = 0.0f,
                f32 w = 1.0f
            ) : x(x), y(y), z(z), w(w) {}

            constexpr quaternion inversed() const;
            constexpr quaternion & inverse();
            constexpr quaternion operator*(quaternion q) const;
            constexpr quaternion& operator*=(quaternion q);
            constexpr float3 operator *(float3 v) const;
            constexpr f32 dot(quaternion q) const;
            constexpr f32 sqrMagnitude() const;
            quaternion & normalize();
            quaternion normalized() const;
            void toAngleAxis(f32 &outAngle, float3 &outAxis) const;
            static quaternion lookRotation(float3 direction, float3 up = float3::up());
        };

        union float4x4 {
            struct {
                float4 a;
                float4 b;
                float4 c;
                float4 d;
            } rows;

            f32 values[4][4];
            
            constexpr float4x4(
                float4 a = float4{ 1, 0, 0, 0 },
                float4 b = float4{ 0, 1, 0, 0 },
                float4 c = float4{ 0, 0, 1, 0 },
                float4 d = float4{ 0, 0, 0, 1 }
            ) : rows { a,b,c,d } { }

            constexpr float4x4 operator +(float4x4 m) const;
            constexpr float4x4 & operator +=(float4x4 m);
            constexpr float4x4 operator -(float4x4 m) const;
            constexpr float4x4 & operator -=(float4x4 m);
            constexpr float4 getColumn(u8 index) const;
            constexpr float4x4 operator *(f32 s) const;
            constexpr float4x4 & operator *=(f32 s);
            constexpr float4x4 operator /(f32 s) const;
            constexpr float4x4 & operator /=(f32 s);
            constexpr float4x4 operator *(float4x4 m) const;
            constexpr float4x4 & operator *=(float4x4 m);
            constexpr float4 operator *(float4 v) const;
            constexpr float3 operator *(float3 v) const;
            constexpr float3 perspectiveMul(float3 v) const;
            quaternion toRotation() const;
            constexpr static float4x4 fromRotation(quaternion q);
            static float4x4 lookAt(float3 direction, float3 up = float3::up());
        };
        
        // float2
        constexpr float2::operator float3() const {
            return float3(this->x, this->y);
        }
        
        constexpr float2::operator float4() const {
            return float4(this->x, this->y);
        }

        constexpr float2 float2::operator-() {
            return float2(-this->x, -this->y);
        }

        constexpr float2 float2::operator +(float2 b) const {
            return float2(this->x + b.x, this->y + b.y);
        }

        constexpr float2 & float2::operator +=(float2 b) {
            this->x += b.x;
            this->y += b.y;
            return *this;
        }

        constexpr float2 float2::operator -(float2 b) const {
            return float2(this->x - b.x, this->y - b.y);
        }

        constexpr float2 & float2::operator -=(float2 b) {
            this->x -= b.x;
            this->y -= b.y;
            return *this;
        }

        constexpr float2 float2::operator /(f32 scalar) const {
            return float2(this->x / scalar, this->y / scalar);
        }

        constexpr float2 & float2::operator /=(f32 scalar) {
            this->x /= scalar;
            this->y /= scalar;
            return *this;
        }

        constexpr float2 float2::operator *(f32 scalar) const {
            return float2(this->x * scalar, this->y * scalar);
        }

        constexpr float2 & float2::operator *=(f32 scalar) {
            this->x *= scalar;
            this->y *= scalar;
            return *this;
        }

        constexpr f32 float2::sqrMagnitude() const {
            return (this->x * this->x) + (this->y * this->y);
        }

        inline f32 float2::magnitude() const {
            return std::sqrtf((this->x * this->x) + (this->y * this->y));
        }

        inline float2 & float2::normalize() {
            f32 mag  = this->sqrMagnitude();
            f32 root = fisqrt(mag);
            this->x *= root;
            this->y *= root;
            return *this;
        }

        inline float2 float2::normalized() const {
            f32 mag  = this->sqrMagnitude();
            f32 root = fisqrt(mag);
            return float2(
                this->x * root,
                this->y * root
            );
        }

        constexpr f32 float2::dot(float2 v) const {
            return (this->x * v.x) + (this->y * v.y);
        }

        constexpr f32 float2::det(float2 v) const {
            return (this->x * v.x) - (this->y * v.y);
        }

        constexpr f32 float2::dot(float2 a, float2 b) {
            return (a.x * b.x) + (a.y * b.y);
        }

        constexpr f32 float2::det(float2 a, float2 b) {
            return (a.x * b.x) - (a.y * b.y);
        }

        constexpr float2 float2::one() {
            return float2(1.0f, 1.0f);
        }

        constexpr float2 float2::up() {
            return float2(0.0f, 1.0f);
        }

        constexpr float2 float2::right() {
            return float2(1.0f, 0.0f);
        }

        constexpr float2 float2::down() {
            return -up();
        }

        constexpr float2 float2::left() {
            return -right();
        }

        constexpr float2 operator /(f32 scalar, float2 v) {
            return float2(v.x / scalar, v.y / scalar);
        }

        constexpr float2 operator *(f32 scalar, float2 v) {
            return float2(v.x * scalar, v.y * scalar);
        }

        constexpr float2 float2::lerp(float2 a, float2 b, f32 t) {
            return (1 - t) * a + t * b;
        }
        
        constexpr f32 float2::inverseLerp(float2 a, float2 b, float2 c) {
            if (a == b) return 0.0f;
            float2 ab = b - a;
            if (ab.sqrMagnitude() == 0.0f) return 0.0f;
            float2 ac = c - a;
            if (ac.sqrMagnitude() == 0.0f) return 0.0f;
            return ac.dot(ab) / (ab).dot(ab);
        }

        // float3
        constexpr float3::operator float2() const {
            return float2(this->x, this->y);
        }

        constexpr float3::operator float4() const {
            return float4(this->x, this->y, this->z);
        }

        constexpr float3 float3::operator-() const {
            return float3(-this->x, -this->y, -this->z);
        }

        constexpr float3 float3::operator +(float3 b) const {
            return float3(this->x + b.x, this->y + b.y, this->z + b.z);
        }

        constexpr float3 & float3::operator +=(float3 b) {
            this->x += b.x;
            this->y += b.y;
            this->z += b.z;
            return *this;
        }
        
        constexpr float3 float3::operator -(float3 b) const {
            return float3(this->x - b.x, this->y - b.y, this->z - b.z);
        }

        constexpr float3 & float3::operator -=(float3 b) {
            this->x -= b.x;
            this->y -= b.y;
            this->z -= b.z;
            return *this;
        }

        constexpr float3 float3::operator /(f32 scalar) const {
            return float3(this->x / scalar, this->y / scalar, this->z / scalar);
        }

        constexpr float3 & float3::operator /=(f32 scalar) {
            this->x /= scalar;
            this->y /= scalar;
            this->z /= scalar;
            return *this;
        }

        constexpr float3 float3::operator *(f32 scalar) const {
            return float3(this->x * scalar, this->y * scalar, this->z * scalar);
        }

        constexpr float3 & float3::operator *=(f32 scalar) {
            this->x *= scalar;
            this->y *= scalar;
            this->z *= scalar;
            return *this;
        }

        constexpr f32 float3::dot(float3 v) const {
            return (this->x * v.x) + (this->x * v.x) + (this->z * v.z);
        }

        constexpr float3 float3::cross(float3 v) const {
            return float3(
                (this->y * v.z) - (this->z * v.y),
                (this->z * v.x) - (this->x * v.z),
                (this->x * v.y) - (this->y * v.x)
            ); 
        }

        constexpr f32 float3::sqrMagnitude() const {
            return (this->x * this->x) + (this->y * this->y) + (this->z * this->z);
        }

        inline f32 float3::magnitude() const {
            return std::sqrtf((this->x * this->x) + (this->y * this->y) + (this->z * this->z));
        }

        inline float3 & float3::normalize() {
            f32 mag  = this->sqrMagnitude();
            f32 root = fisqrt(mag);
            this->x *= root;
            this->y *= root;
            this->z *= root;
            return *this;
        }

        inline float3 float3::normalized() const {
            f32 mag  = this->sqrMagnitude();
            f32 root = fisqrt(mag);
            return float3(
                this->x * root,
                this->y * root,
                this->z * root
            );
        }

        constexpr f32 float3::dot(float3 a, float3 b) {
            return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
        }

        constexpr float3 float3::cross(float3 a, float3 b) {
            return float3(
                (a.y * b.z) - (a.z * b.y),
                (a.z * b.x) - (a.x * b.z),
                (a.x * b.y) - (a.y * b.x)
            ); 
        }

        constexpr float3 float3::one() {
            return float3(1.0f, 1.0f, 1.0f);
        }

        constexpr float3 float3::up() {
            return float3(0.0f, 1.0f, 0.0f);
        }

        constexpr float3 float3::right() {
            return float3(1.0f, 0.0f, 0.0f);
        }

        constexpr float3 float3::down() {
            return -up();
        }

        constexpr float3 float3::left() {
            return -right();
        }

        constexpr float3 float3::forward() {
            return float3(0.0f, 0.0f, -1.0f);
        }

        constexpr float3 float3::back() {
            return -forward();
        }

        constexpr float3 operator /(f32 scalar, float3 v) {
            return float3(v.x / scalar, v.y / scalar, v.z / scalar);
        }

        constexpr float3 operator *(f32 scalar, float3 v) {
            return float3(v.x * scalar, v.y * scalar, v.z * scalar);
        }

        constexpr float3 float3::lerp(float3 a, float3 b, f32 t) {
            return (1 - t) * a + t * b;
        }
        
        constexpr f32 float3::inverseLerp(float3 a, float3 b, float3 c) {
            if (a == b) return 0.0f;
            float3 ab = b - a;
            if (ab.sqrMagnitude() == 0.0f) return 0.0f;
            float3 ac = c - a;
            if (ac.sqrMagnitude() == 0.0f) return 0.0f;
            return ac.dot(ab) / (ab).dot(ab);
        }

        // float4
        constexpr float4::operator float2() const {
            return float2(this->x, this->y);
        }

        constexpr float4::operator float3() const {
            return float3(this->x, this->y, this->z);
        }

        constexpr float4 float4::operator -() const {
            return float4(-this->x, -this->y, -this->z, -this->w);
        }

        constexpr float4 float4::operator +(float4 b) const {
            return float4(this->x + b.x, this->y + b.y, this->z + b.z, this->w + b.w);
        }

        constexpr float4 & float4::operator +=(float4 b) {
            this->x += b.x;
            this->y += b.y;
            this->z += b.z;
            this->w += b.w;
            return *this;
        }

        constexpr float4 float4::operator -(float4 b) const {
            return float4(this->x - b.x, this->y - b.y, this->z - b.z, this->w - b.w);
        }

        constexpr float4 & float4::operator -=(float4 b) {
            this->x -= b.x;
            this->y -= b.y;
            this->z -= b.z;
            this->w -= b.w;
            return *this;
        }

        constexpr float4 float4::operator /(f32 scalar) const {
            return float4(this->x / scalar, this->y / scalar, this->z / scalar, this->w / scalar);
        }

        constexpr float4 & float4::operator /=(f32 scalar) {
            this->x /= scalar;
            this->y /= scalar;
            this->z /= scalar;
            this->w /= scalar;
            return *this;
        }

        constexpr float4 float4::operator *(f32 scalar) const {
            return float4(this->x * scalar, this->y * scalar, this->z * scalar, this->w * scalar);
        }

        constexpr float4 & float4::operator *=(f32 scalar) {
            this->x *= scalar;
            this->y *= scalar;
            this->z *= scalar;
            this->w *= scalar;
            return *this;
        }

        constexpr f32 float4::dot(float4 v) const {
            return (this->x * v.x) + (this->y * v.y) + (this->z * v.z) + (this->w * v.w);
        }

        constexpr f32 float4::sqrMagnitude() const {
            return (this->x * this->x) + (this->y * this->y) + (this->z * this->z) + (this->w * this->w);
        }

        inline f32 float4::magnitude() const {
            return std::sqrtf((this->x * this->x) + (this->y * this->y) + (this->z * this->z) + (this->w * this->w));
        }

        inline float4 & float4::normalize() {
            f32 mag  = this->sqrMagnitude();
            f32 root = fisqrt(mag);
            this->x *= root;
            this->y *= root;
            this->z *= root;
            this->w *= root;
            return *this;
        }

        inline float4 float4::normalized() const {
            f32 mag  = this->sqrMagnitude();
            f32 root = fisqrt(mag);
            return float4(
                this->x * root,
                this->y * root,
                this->z * root,
                this->w * root
            );
        }

        constexpr f32 float4::dot(float4 a, float4 b) {
            return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w);
        }

        constexpr float4 operator /(f32 scalar, float4 v) {
            return float4(v.x / scalar, v.y / scalar, v.z / scalar, v.w / scalar);
        }

        constexpr float4 operator *(f32 scalar, float4 v) {
            return float4(v.x * scalar, v.y * scalar, v.z * scalar, v.w * scalar);
        }

        constexpr float4 float4::lerp(float4 a, float4 b, f32 t) {
            return (1 - t) * a + t * b;
        }
        
        constexpr f32 float4::inverseLerp(float4 a, float4 b, float4 c) {
            if (a == b) return 0.0f;
            float4 ab = b - a;
            if (ab.sqrMagnitude() == 0.0f) return 0.0f;
            float4 ac = c - a;
            if (ac.sqrMagnitude() == 0.0f) return 0.0f;
            return ac.dot(ab) / (ab).dot(ab);
        }

        constexpr quaternion quaternion::inversed() const {
            return quaternion(-x, -y, -z, w);
        }

        constexpr quaternion & quaternion::inverse() {
            this->x *= -1;
            this->y *= -1;
            this->z *= -1;
            return *this;
        }
        
        constexpr quaternion quaternion::operator*(quaternion q) const {
            return quaternion(
                this->w * q.x + this->x * q.w + this->y * q.z - this->z * q.y, 
                this->w * q.y + this->y * q.w + this->z * q.x - this->x * q.z, 
                this->w * q.z + this->z * q.w + this->x * q.y - this->y * q.x, 
                this->w * q.w - this->x * q.x - this->y * q.y - this->z * q.z
            );
        }
        
        constexpr quaternion& quaternion::operator*=(quaternion q) {
            this->x = this->w * q.x + this->x * q.w + this->y * q.z - this->z * q.y; 
            this->y = this->w * q.y + this->y * q.w + this->z * q.x - this->x * q.z; 
            this->z = this->w * q.z + this->z * q.w + this->x * q.y - this->y * q.x; 
            this->w = this->w * q.w - this->x * q.x - this->y * q.y - this->z * q.z;
            return *this;
        }

        constexpr float3 quaternion::operator *(float3 v) const {
            f32 x = this->x * 2.0f;
            f32 y = this->y * 2.0f;
            f32 z = this->z * 2.0f;
            f32 xx = this->x * x;
            f32 yy = this->y * y;
            f32 zz = this->z * z;
            f32 xy = this->x * y;
            f32 xz = this->x * z;
            f32 yz = this->y * z;
            f32 wx = this->w * x;
            f32 wy = this->w * y;
            f32 wz = this->w * z;
            
            return float3 {
                (1.0f - (yy + zz)) * v.x + (xy - wz) * v.y + (xz + wy) * v.z,
                (xy + wz) * v.x + (1.0f - (xx + zz)) * v.y + (yz - wx) * v.z,
                (xz - wy) * v.x + (yz + wx) * v.y + (1.0f - (xx + yy)) * v.z,
            };
        }

        constexpr f32 quaternion::dot(quaternion q) const {
            return this->x * q.x + this->y * q.y + this->z * q.z + this->w * q.w;
        }

        constexpr f32 quaternion::sqrMagnitude() const {
            return this->x * this->x + this->y * this->y + this->z * this->z + this->w * this->w;
        }
        
        inline quaternion & quaternion::normalize() {
            f32 mag = sqrMagnitude();
            f32 root = fisqrt(mag);
            this->x *= root;
            this->y *= root;
            this->z *= root;
            this->w *= root;
            return *this;
        }

        inline quaternion quaternion::normalized() const {
            f32 mag = sqrMagnitude();
            f32 root = fisqrt(mag);
            return quaternion(
                this->x * root,
                this->y * root,
                this->z * root,
                this->w * root
            );
        }

        inline void quaternion::toAngleAxis(f32 &outAngle, float3 &outAxis) const {
            float3 axis{ this->x, this->y, this->z };
            axis.normalize();
            f32 mag = axis.magnitude();
            if (mag == 0.0f) {
                outAxis = float3(1.0f);
            } else {
                outAxis = axis;
            }
            outAngle = 2 * atan2f(mag, this->w);
        }

        inline quaternion quaternion::lookRotation(float3 direction, float3 up) {
            return float4x4::lookAt(direction, up).toRotation();
        }
        
        constexpr float4x4 float4x4::operator +(float4x4 m) const {
            return float4x4(
                this->rows.a + m.rows.a,
                this->rows.b + m.rows.b,
                this->rows.c + m.rows.c,
                this->rows.d + m.rows.d
            );
        }

        constexpr float4x4 & float4x4::operator +=(float4x4 m) {
            this->rows.a += m.rows.a;
            this->rows.b += m.rows.b;
            this->rows.c += m.rows.c;
            this->rows.d += m.rows.d;
            return *this;
        }

        constexpr float4x4 float4x4::operator -(float4x4 m) const {
            return float4x4(
                this->rows.a - m.rows.a,
                this->rows.b - m.rows.b,
                this->rows.c - m.rows.c,
                this->rows.d - m.rows.d
            );
        }

        constexpr float4x4 & float4x4::operator -=(float4x4 m) {
            this->rows.a -= m.rows.a;
            this->rows.b -= m.rows.b;
            this->rows.c -= m.rows.c;
            this->rows.d -= m.rows.d;
            return *this;
        }

        constexpr float4 float4x4::getColumn(u8 index) const {
            index = clampi(index, 0, 3);
            return float4 {
                this->values[0][index],
                this->values[1][index],
                this->values[2][index],
                this->values[3][index],
            };
        }

        constexpr float4x4 float4x4::operator *(f32 s) const {
            return float4x4 {
                this->rows.a * s,
                this->rows.b * s,
                this->rows.c * s,
                this->rows.d * s,
            };
        }

        constexpr float4x4 & float4x4::operator *=(f32 s) {
            this->rows.a *= s;
            this->rows.b *= s;
            this->rows.c *= s;
            this->rows.d *= s;
            return *this;
        }

        constexpr float4x4 float4x4::operator /(f32 s) const {
            return float4x4 {
                this->rows.a / s,
                this->rows.b / s,
                this->rows.c / s,
                this->rows.d / s,
            };
        }

        constexpr float4x4 & float4x4::operator /=(f32 s) {
            this->rows.a /= s;
            this->rows.b /= s;
            this->rows.c /= s;
            this->rows.d /= s;
            return *this;
        }

        constexpr float4x4 float4x4::operator *(float4x4 m) const {
            return float4x4 {
                float4 {
                    this->rows.a.dot(m.getColumn(0)),
                    this->rows.a.dot(m.getColumn(1)),
                    this->rows.a.dot(m.getColumn(2)),
                    this->rows.a.dot(m.getColumn(3)),
                },
                float4 {
                    this->rows.b.dot(m.getColumn(0)),
                    this->rows.b.dot(m.getColumn(1)),
                    this->rows.b.dot(m.getColumn(2)),
                    this->rows.b.dot(m.getColumn(3)),
                },
                float4 {
                    this->rows.c.dot(m.getColumn(0)),
                    this->rows.c.dot(m.getColumn(1)),
                    this->rows.c.dot(m.getColumn(2)),
                    this->rows.c.dot(m.getColumn(3)),
                },
                float4 {
                    this->rows.d.dot(m.getColumn(0)),
                    this->rows.d.dot(m.getColumn(1)),
                    this->rows.d.dot(m.getColumn(2)),
                    this->rows.d.dot(m.getColumn(3)),
                },
            };
        }

        constexpr float4x4 & float4x4::operator *=(float4x4 m) {
            this->rows.a = float4 {
                this->rows.a.dot(m.getColumn(0)),
                this->rows.a.dot(m.getColumn(1)),
                this->rows.a.dot(m.getColumn(2)),
                this->rows.a.dot(m.getColumn(3)),
            };
            this->rows.b = float4 {
                this->rows.b.dot(m.getColumn(0)),
                this->rows.b.dot(m.getColumn(1)),
                this->rows.b.dot(m.getColumn(2)),
                this->rows.b.dot(m.getColumn(3)),
            };
            this->rows.c = float4 {
                this->rows.c.dot(m.getColumn(0)),
                this->rows.c.dot(m.getColumn(1)),
                this->rows.c.dot(m.getColumn(2)),
                this->rows.c.dot(m.getColumn(3)),
            };
            this->rows.d = float4 {
                this->rows.d.dot(m.getColumn(0)),
                this->rows.d.dot(m.getColumn(1)),
                this->rows.d.dot(m.getColumn(2)),
                this->rows.d.dot(m.getColumn(3)),
            };

            return *this;
        }

        constexpr float4 float4x4::operator *(float4 v) const {
            return (
                v.x * this->rows.a +
                v.y * this->rows.b +
                v.z * this->rows.c +
                v.w * this->rows.d
            );
        }

        constexpr float3 float4x4::operator *(float3 v) const {
            return (float3) this->operator*((float4) v);
        }

        constexpr float3 float4x4::perspectiveMul(float3 v) const {
            float3 result = float3 {
                this->rows.a.dot(v),
                this->rows.b.dot(v),
                this->rows.c.dot(v),
            };
            f32 w = this->rows.d.dot(v);
            w = 1.0f / w;
            result.x *= w;
            result.y *= w;
            result.z *= w;
            return result;
        }
        
        constexpr float4x4 float4x4::fromRotation(quaternion q) {
            f32 x2 = q.x + q.x;
            f32 y2 = q.y + q.y;
            f32 z2 = q.z + q.z;

            f32 yy = q.y * y2;
            f32 xy = q.x * y2;
            f32 xz = q.x * z2;
            f32 yz = q.y * z2;

            f32 zz = q.z * z2;
            f32 wz = q.w * z2;
            f32 wy = q.w * y2;
            f32 wx = q.w * x2;

            f32 xx = q.x * x2;

            return float4x4 {
                float4 { -yy - zz + 1.0f,         xy + wz,         xz - wy, 0.0f },
                float4 {         xy - wz, -xx - zz + 1.0f,         yz + wx, 0.0f },
                float4 {         xz + wy,         yz - wx, -xx - yy + 1.0f, 0.0f },
                float4 {            0.0f,            0.0f,            0.0f, 1.0f },
            };
        }

        inline quaternion float4x4::toRotation() const {
            f32 trace = this->values[0][0] + this->values[1][1] + this->values[2][2];
            if (trace > 0) {
                f32 root = fisqrt(trace + 1.0f) * 0.5f;
                return quaternion {
                    (this->values[1][2] - this->values[2][1]) * root,
                    (this->values[2][0] - this->values[0][2]) * root,
                    (this->values[0][1] - this->values[1][0]) * root,
                    root * (trace + 1.0f),
                };
            } else if (this->values[0][0] > this->values[1][1] && this->values[0][0] > this->values[2][2]) {
                f32 trace1 = this->values[0][0] - this->values[1][1] - this->values[2][2] + 1.0f;
                f32 root = fisqrt(trace1) * 0.5f;
                return quaternion {
                    root * trace1,
                    (this->values[0][1] + this->values[1][0]) * root,
                    (this->values[2][0] + this->values[0][2]) * root,
                    (this->values[1][2] - this->values[2][1]) * root,
                };
            } else if (this->values[1][1] > this->values[2][2]) {
                f32 trace1 = -this->values[0][0] + this->values[1][1] - this->values[2][2] + 1.0f;
                f32 root = fisqrt(trace1) * 0.5f;
                return quaternion {
                    (this->values[0][1] + this->values[1][0]) * root,
                    root * trace1,
                    (this->values[1][2] + this->values[2][1]) * root,
                    (this->values[2][0] - this->values[0][2]) * root,
                };
            }
            
            f32 trace1 = -this->values[0][0] - this->values[1][1] + this->values[2][2] + 1.0f;
            f32 root = fisqrt(trace1) * 0.5f;
            return quaternion {
                (this->values[2][0] + this->values[0][2]) * root,
                (this->values[1][2] + this->values[2][1]) * root,
                root * trace1,
                (this->values[0][1] - this->values[1][0]) * root,
            };
        }

        inline float4x4 float4x4::lookAt(float3 direction, float3 up) {
            float3 z = direction.normalized();
            float3 x = up.cross(z).normalized();
            float3 y = z.cross(x);
            return float4x4 { x, y, z };
        }
    }
}

#endif

