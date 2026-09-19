#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <string>
#include <utility>

namespace bsp {

enum class LogLevel { Error = 0, Warn = 1, Info = 2, Debug = 3, Trace = 4 };

inline LogLevel& current_level() {
    static LogLevel level = LogLevel::Info;
    return level;
}

inline bool& use_prefix() {
    static bool p = false;
    return p;
}

inline void set_log_level(LogLevel l) { current_level() = l; }
inline void set_log_prefix(bool p) { use_prefix() = p; }

inline bool parse_log_level(const std::string& s, LogLevel& out) {
    if (s == "error") { out = LogLevel::Error; return true; }
    if (s == "warn") { out = LogLevel::Warn; return true; }
    if (s == "info") { out = LogLevel::Info; return true; }
    if (s == "debug") { out = LogLevel::Debug; return true; }
    if (s == "trace") { out = LogLevel::Trace; return true; }
    return false;
}

struct LogSink {
    bool enabled;
    std::ostream& os;
    LogSink(bool e, std::ostream& o) : enabled(e), os(o) {}
    template <typename T>
    LogSink& operator<<(T&& v) {
        if (enabled) os << std::forward<T>(v);
        return *this;
    }
    LogSink& operator<<(std::ostream& (*manip)(std::ostream&)) {
        if (enabled) os << manip;
        return *this;
    }
};

inline LogSink log_at(LogLevel l, std::ostream& os) {
    return LogSink(static_cast<int>(current_level()) >= static_cast<int>(l), os);
}

}  // namespace bsp

#define BSP_ERROR (bsp::log_at(bsp::LogLevel::Error, std::cerr))
#define BSP_WARN  (bsp::log_at(bsp::LogLevel::Warn, std::cerr))
#define BSP_INFO  (bsp::log_at(bsp::LogLevel::Info, std::cout))
#define BSP_DEBUG (bsp::log_at(bsp::LogLevel::Debug, std::cout))
#define BSP_TRACE (bsp::log_at(bsp::LogLevel::Trace, std::cout))

#endif
