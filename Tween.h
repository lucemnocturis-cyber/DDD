#pragma once

#include <cmath>
#include <functional>

namespace DDD {

/**
 * Easing functions for smooth animations
 */
namespace Easing {
    
    // Linear (no easing)
    inline float Linear(float t) {
        return t;
    }
    
    // Quadratic
    inline float QuadIn(float t) {
        return t * t;
    }
    
    inline float QuadOut(float t) {
        return t * (2.0f - t);
    }
    
    inline float QuadInOut(float t) {
        return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
    }
    
    // Cubic
    inline float CubicIn(float t) {
        return t * t * t;
    }
    
    inline float CubicOut(float t) {
        float f = t - 1.0f;
        return f * f * f + 1.0f;
    }
    
    inline float CubicInOut(float t) {
        return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
    }
    
    // Quartic
    inline float QuartOut(float t) {
        float f = t - 1.0f;
        return 1.0f - f * f * f * f;
    }
    
    // Elastic
    inline float ElasticOut(float t) {
        if (t == 0.0f || t == 1.0f) return t;
        return std::pow(2.0f, -10.0f * t) * std::sin((t - 0.1f) * 5.0f * 3.14159f) + 1.0f;
    }
    
    // Back (overshoot)
    inline float BackOut(float t) {
        const float c1 = 1.70158f;
        const float c3 = c1 + 1.0f;
        float f = t - 1.0f;
        return 1.0f + c3 * f * f * f + c1 * f * f;
    }
    
    inline float BackIn(float t) {
        const float c1 = 1.70158f;
        const float c3 = c1 + 1.0f;
        return c3 * t * t * t - c1 * t * t;
    }
    
    // Bounce
    inline float BounceOut(float t) {
        const float n1 = 7.5625f;
        const float d1 = 2.75f;
        
        if (t < 1.0f / d1) {
            return n1 * t * t;
        } else if (t < 2.0f / d1) {
            t -= 1.5f / d1;
            return n1 * t * t + 0.75f;
        } else if (t < 2.5f / d1) {
            t -= 2.25f / d1;
            return n1 * t * t + 0.9375f;
        } else {
            t -= 2.625f / d1;
            return n1 * t * t + 0.984375f;
        }
    }
    
    // Sine
    inline float SineOut(float t) {
        return std::sin(t * 3.14159f * 0.5f);
    }
    
    inline float SineInOut(float t) {
        return 0.5f * (1.0f - std::cos(3.14159f * t));
    }
    
    // Expo
    inline float ExpoOut(float t) {
        return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
    }
}

/**
 * Tween - animates a value over time
 */
class Tween {
public:
    using EaseFunc = std::function<float(float)>;
    using CompleteCallback = std::function<void()>;
    
    Tween() = default;
    
    /**
     * Start a new tween
     */
    void Start(float from, float to, float duration, EaseFunc easing = Easing::Linear) {
        m_from = from;
        m_to = to;
        m_duration = duration;
        m_elapsed = 0.0f;
        m_easing = easing;
        m_active = true;
        m_value = from;
    }
    
    /**
     * Update the tween
     */
    void Update(float deltaTime) {
        if (!m_active) return;
        
        m_elapsed += deltaTime;
        float t = m_elapsed / m_duration;
        
        if (t >= 1.0f) {
            t = 1.0f;
            m_active = false;
            m_value = m_to;
            
            if (m_onComplete) {
                m_onComplete();
            }
        } else {
            float easedT = m_easing ? m_easing(t) : t;
            m_value = m_from + (m_to - m_from) * easedT;
        }
    }
    
    /**
     * Get current value
     */
    float GetValue() const { return m_value; }
    
    /**
     * Check if tween is active
     */
    bool IsActive() const { return m_active; }
    
    /**
     * Set completion callback
     */
    void SetOnComplete(CompleteCallback callback) { m_onComplete = callback; }
    
    /**
     * Stop the tween
     */
    void Stop() { m_active = false; }
    
    /**
     * Force complete
     */
    void Complete() {
        m_active = false;
        m_value = m_to;
        if (m_onComplete) m_onComplete();
    }
    
private:
    float m_from = 0.0f;
    float m_to = 0.0f;
    float m_duration = 1.0f;
    float m_elapsed = 0.0f;
    float m_value = 0.0f;
    bool m_active = false;
    EaseFunc m_easing;
    CompleteCallback m_onComplete;
};

/**
 * AnimatedValue - a value that smoothly transitions to targets
 */
class AnimatedValue {
public:
    AnimatedValue(float initial = 0.0f) : m_current(initial), m_target(initial) {}
    
    /**
     * Set target value with animation
     */
    void SetTarget(float target, float duration = 0.3f, Tween::EaseFunc easing = Easing::QuadOut) {
        if (target == m_target && !m_tween.IsActive()) return;
        m_target = target;
        m_tween.Start(m_current, target, duration, easing);
    }
    
    /**
     * Set value immediately (no animation)
     */
    void SetImmediate(float value) {
        m_current = value;
        m_target = value;
        m_tween.Stop();
    }
    
    /**
     * Update animation
     */
    void Update(float deltaTime) {
        m_tween.Update(deltaTime);
        if (m_tween.IsActive()) {
            m_current = m_tween.GetValue();
        }
    }
    
    /**
     * Get current value
     */
    float Get() const { return m_current; }
    
    /**
     * Get target value
     */
    float GetTarget() const { return m_target; }
    
    /**
     * Check if animating
     */
    bool IsAnimating() const { return m_tween.IsActive(); }
    
    /**
     * Implicit conversion to float
     */
    operator float() const { return m_current; }
    
private:
    float m_current;
    float m_target;
    Tween m_tween;
};

/**
 * Counter - animated number counter with formatting
 */
class AnimatedCounter {
public:
    AnimatedCounter(int initial = 0) : m_displayValue(static_cast<float>(initial)), m_actualValue(initial) {}
    
    /**
     * Set the value (animates to it)
     */
    void SetValue(int value, float duration = 0.5f) {
        if (value == m_actualValue) return;
        m_actualValue = value;
        m_displayValue.SetTarget(static_cast<float>(value), duration, Easing::QuadOut);
    }
    
    /**
     * Add to value (with animation)
     */
    void Add(int amount, float duration = 0.5f) {
        SetValue(m_actualValue + amount, duration);
    }
    
    /**
     * Set immediately
     */
    void SetImmediate(int value) {
        m_actualValue = value;
        m_displayValue.SetImmediate(static_cast<float>(value));
    }
    
    /**
     * Update
     */
    void Update(float deltaTime) {
        m_displayValue.Update(deltaTime);
    }
    
    /**
     * Get display value (for rendering)
     */
    int GetDisplayValue() const { return static_cast<int>(m_displayValue.Get()); }
    
    /**
     * Get actual value
     */
    int GetActualValue() const { return m_actualValue; }
    
    /**
     * Check if animating
     */
    bool IsAnimating() const { return m_displayValue.IsAnimating(); }
    
private:
    AnimatedValue m_displayValue;
    int m_actualValue;
};

} // namespace DDD
