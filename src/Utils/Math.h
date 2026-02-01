#pragma once

#include <cmath>
#include <algorithm>

namespace DDD {

/**
 * 2D position/vector
 */
struct Position {
    int x = 0;
    int y = 0;
    
    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
    
    bool operator!=(const Position& other) const {
        return !(*this == other);
    }
    
    Position operator+(const Position& other) const {
        return {x + other.x, y + other.y};
    }
    
    Position operator-(const Position& other) const {
        return {x - other.x, y - other.y};
    }
};

/**
 * Floating point vector
 */
struct Vector2f {
    float x = 0.0f;
    float y = 0.0f;
    
    Vector2f operator+(const Vector2f& other) const {
        return {x + other.x, y + other.y};
    }
    
    Vector2f operator-(const Vector2f& other) const {
        return {x - other.x, y - other.y};
    }
    
    Vector2f operator*(float scalar) const {
        return {x * scalar, y * scalar};
    }
    
    float Length() const {
        return std::sqrt(x * x + y * y);
    }
    
    Vector2f Normalized() const {
        float len = Length();
        if (len > 0.0001f) {
            return {x / len, y / len};
        }
        return {0.0f, 0.0f};
    }
};

/**
 * Rectangle
 */
struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    
    bool Contains(int px, int py) const {
        return px >= x && px < x + width && py >= y && py < y + height;
    }
    
    bool Contains(const Position& pos) const {
        return Contains(pos.x, pos.y);
    }
    
    bool Intersects(const Rect& other) const {
        return x < other.x + other.width &&
               x + width > other.x &&
               y < other.y + other.height &&
               y + height > other.y;
    }
};

/**
 * Math utilities
 */
namespace Math {
    
    inline int Clamp(int value, int min, int max) {
        return std::max(min, std::min(max, value));
    }
    
    inline float Clamp(float value, float min, float max) {
        return std::max(min, std::min(max, value));
    }
    
    inline float Lerp(float a, float b, float t) {
        return a + (b - a) * Clamp(t, 0.0f, 1.0f);
    }
    
    inline int ManhattanDistance(const Position& a, const Position& b) {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y);
    }
    
    inline float EuclideanDistance(const Position& a, const Position& b) {
        float dx = static_cast<float>(a.x - b.x);
        float dy = static_cast<float>(a.y - b.y);
        return std::sqrt(dx * dx + dy * dy);
    }
    
    // Easing functions for animations
    inline float EaseInQuad(float t) {
        return t * t;
    }
    
    inline float EaseOutQuad(float t) {
        return t * (2.0f - t);
    }
    
    inline float EaseInOutQuad(float t) {
        return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
    }
}

} // namespace DDD
