#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <memory>
#include <string>

namespace DDD {

/**
 * Logging utilities using spdlog
 */
class Logger {
public:
    /**
     * Initialize the logging system
     */
    static void Initialize() {
        // Console sink with colors
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_level(spdlog::level::trace);
        
        // File sink
        auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("game.log", true);
        fileSink->set_level(spdlog::level::debug);
        
        // Create multi-sink logger
        std::vector<spdlog::sink_ptr> sinks{consoleSink, fileSink};
        s_logger = std::make_shared<spdlog::logger>("DDD", sinks.begin(), sinks.end());
        
        s_logger->set_level(spdlog::level::trace);
        s_logger->set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
        
        spdlog::register_logger(s_logger);
    }
    
    /**
     * Shutdown logging
     */
    static void Shutdown() {
        spdlog::shutdown();
    }
    
    // Logging methods with format support
    template<typename... Args>
    static void Trace(const std::string& fmt, Args&&... args) {
        if (s_logger) s_logger->trace(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void Debug(const std::string& fmt, Args&&... args) {
        if (s_logger) s_logger->debug(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void Info(const std::string& fmt, Args&&... args) {
        if (s_logger) s_logger->info(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void Warning(const std::string& fmt, Args&&... args) {
        if (s_logger) s_logger->warn(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void Error(const std::string& fmt, Args&&... args) {
        if (s_logger) s_logger->error(fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void Critical(const std::string& fmt, Args&&... args) {
        if (s_logger) s_logger->critical(fmt, std::forward<Args>(args)...);
    }
    
private:
    static std::shared_ptr<spdlog::logger> s_logger;
};

} // namespace DDD
