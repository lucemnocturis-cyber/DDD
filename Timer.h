#pragma once

#include <chrono>
#include <functional>

namespace DDD {

/**
 * High-resolution timer for profiling and timing
 */
class Timer {
public:
    Timer() : m_start(std::chrono::high_resolution_clock::now()) {}
    
    /**
     * Reset the timer
     */
    void Reset() {
        m_start = std::chrono::high_resolution_clock::now();
    }
    
    /**
     * Get elapsed time in seconds
     */
    float ElapsedSeconds() const {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - m_start);
        return duration.count() / 1000000.0f;
    }
    
    /**
     * Get elapsed time in milliseconds
     */
    float ElapsedMilliseconds() const {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - m_start);
        return duration.count() / 1000.0f;
    }
    
    /**
     * Get elapsed time in microseconds
     */
    long long ElapsedMicroseconds() const {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - m_start);
        return duration.count();
    }
    
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};

/**
 * Scoped timer that reports elapsed time on destruction
 */
class ScopedTimer {
public:
    ScopedTimer(const std::string& name, std::function<void(const std::string&, float)> callback = nullptr)
        : m_name(name)
        , m_callback(callback)
    {
    }
    
    ~ScopedTimer() {
        float elapsed = m_timer.ElapsedMilliseconds();
        if (m_callback) {
            m_callback(m_name, elapsed);
        }
    }
    
private:
    std::string m_name;
    Timer m_timer;
    std::function<void(const std::string&, float)> m_callback;
};

} // namespace DDD
