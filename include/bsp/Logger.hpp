//
//  Logger.hpp
//  Minimal logging for BSP (levels + stream-style macros). Does not affect the algorithm.
//
#ifndef BSP_Logger_h
#define BSP_Logger_h

#include <iostream>
#include <string>

namespace bsp {

enum class LogLevel {
    Error = 0,
    Warn  = 1,
    Info  = 2,
    Debug = 3,
    Trace = 4
};

inline LogLevel &log_level()
{
    static LogLevel level = LogLevel::Info;
    return level;
}

inline void set_log_level(LogLevel level)
{
    log_level() = level;
}

inline bool log_enabled(LogLevel level)
{
    return static_cast<int>(level) <= static_cast<int>(log_level());
}

inline bool parse_log_level(const std::string &name, LogLevel &out)
{
    if (name == "error") { out = LogLevel::Error; return true; }
    if (name == "warn" || name == "warning") { out = LogLevel::Warn; return true; }
    if (name == "info") { out = LogLevel::Info; return true; }
    if (name == "debug") { out = LogLevel::Debug; return true; }
    if (name == "trace") { out = LogLevel::Trace; return true; }
    return false;
}

inline const char *log_level_name(LogLevel level)
{
    switch (level) {
    case LogLevel::Error: return "ERROR";
    case LogLevel::Warn:  return "WARN";
    case LogLevel::Info:  return "INFO";
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Trace: return "TRACE";
    }
    return "LOG";
}

inline bool &log_prefix_enabled()
{
    static bool on = false;
    return on;
}

inline void set_log_prefix(bool on)
{
    log_prefix_enabled() = on;
}

inline std::ostream &log_ostream(LogLevel level)
{
    return (level <= LogLevel::Warn) ? std::cerr : std::cout;
}

/* Stream sink that discards when the level is disabled. */
class LogSink {
public:
    LogSink(LogLevel level, bool active, std::ostream &os)
        : level_(level), active_(active), os_(os), prefix_done_(false)
    {}

    /* Non-const: Graph/Clause operator<< take mutable references. */
    template <typename T>
    LogSink &operator<<(T &value)
    {
        if (!active_)
            return *this;
        write_prefix_once();
        os_ << value;
        return *this;
    }

    template <typename T>
    LogSink &operator<<(const T &value)
    {
        if (!active_)
            return *this;
        write_prefix_once();
        os_ << value;
        return *this;
    }

    LogSink &operator<<(std::ostream &(*manip)(std::ostream &))
    {
        if (!active_)
            return *this;
        write_prefix_once();
        os_ << manip;
        return *this;
    }

private:
    void write_prefix_once()
    {
        if (prefix_done_ || !log_prefix_enabled())
            return;
        os_ << "[" << log_level_name(level_) << "] ";
        prefix_done_ = true;
    }

    LogLevel level_;
    bool active_;
    std::ostream &os_;
    bool prefix_done_;
};

inline LogSink log(LogLevel level)
{
    return LogSink(level, log_enabled(level), log_ostream(level));
}

inline LogSink log_error() { return log(LogLevel::Error); }
inline LogSink log_warn()  { return log(LogLevel::Warn); }
inline LogSink log_info()  { return log(LogLevel::Info); }
inline LogSink log_debug() { return log(LogLevel::Debug); }
inline LogSink log_trace() { return log(LogLevel::Trace); }

} // namespace bsp

#define BSP_ERROR bsp::log_error()
#define BSP_WARN  bsp::log_warn()
#define BSP_INFO  bsp::log_info()
#define BSP_DEBUG bsp::log_debug()
#define BSP_TRACE bsp::log_trace()

#endif /* BSP_Logger_h */
