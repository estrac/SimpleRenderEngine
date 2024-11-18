/*
 *  SimpleRenderEngine (https://github.com/mortennobel/SimpleRenderEngine)
 *
 *  Created by Morten Nobel-Jørgensen ( http://www.nobel-joergensen.com/ )
 *  License: MIT
 */
#pragma once

#include <string>
#include <functional>
#include <sstream>
#include <filesystem>

// Logging and assertions in SRE are done using the preprocessor macros:
//
// LOG_VERBOSE(format, ...)
// LOG_INFO(format, ...)
// LOG_WARNING(format, ...)
// LOG_ERROR(format, ...)
// LOG_FATAL(format, ...)
// LOG_ASSERT(condition)
//
// Each function (except LOG_ASSERT) works similar to printf, e.g.
// LOG_INFO("Hello %s. Meaning of life: %i", "world, 42); // prints "Hello world Meaning of life: 42"
//
// If the symbol SRE_LOG_DISABLED is defined all logging is disabled
//
// The default behavior (defined in the logHandler - which may be overwritten) is verbose logging is ignored,
// failed asserts and fatal errors throw a std::runtime_error
//
namespace sre{
    enum class LogType {
        Verbose,
        Info,
        Warning,
        Error,
        Fatal,
        Assert
    };

    class Log {
    private:
        static inline bool isSetup = false;
        static inline bool isVerbose = false;
        static inline std::filesystem::path logArchivePath;
        static inline std::filesystem::path logPath;
    public:
        static void Setup(const bool& verbose = false);
        static bool IsSetup() {return isSetup;}
        static bool IsVerbose() {return isVerbose;}
        static void verbose(const char * function,const char * file, int line, const char * format, ...);
        static void info(const char * function,const char * file, int line, const char * format, ...);
        static void warning(const char * function,const char * file, int line, const char * format, ...);
        static void error(const char * function,const char * file, int line, const char * format, ...);
        static void fatal(const char * function,const char * file, int line, const char * format, ...);
        static void sreAssert(const char * function,const char * file, int line, std::string msg);

        static std::function<void(const char * function,const char * file, int line, LogType type, std::string msg)> logHandler;
    };
}
// LOG_FATAL should be used to stop execution for good reason, hence, keep it defined even if log is disabled
#ifdef SRE_LOG_DISABLED
    #define LOG_VERBOSE(X, ...)
    #define LOG_INFO(X, ...)
    #define LOG_WARNING(X, ...)
    #define LOG_ERROR(X, ...)
    #define LOG_FATAL(X, ...) sre::Log::fatal(LOG_LOCATION, X,##  __VA_ARGS__)
    #define LOG_ASSERT(condition)
#else
    #define LOG_LOCATION __func__, __FILE__,__LINE__
    #define LOG_VERBOSE(X, ...) {                                              \
        if (sre::Log::IsVerbose()) {                                           \
            sre::Log::verbose(LOG_LOCATION, X,##  __VA_ARGS__);                \
        }                                                                      \
    }
    #define LOG_INFO(X, ...) sre::Log::info(LOG_LOCATION, X, ## __VA_ARGS__)
    #define LOG_WARNING(X, ...) sre::Log::warning(LOG_LOCATION, X, ## __VA_ARGS__)
    #define LOG_ERROR(X, ...) sre::Log::error(LOG_LOCATION, X,##  __VA_ARGS__)
    #define LOG_FATAL(X, ...) sre::Log::fatal(LOG_LOCATION, X,##  __VA_ARGS__)
    #define LOG_ASSERT(condition) {                                            \
        if (!(condition)) {                                                    \
            std::ostringstream _sre_log_assert_msg_;                           \
            _sre_log_assert_msg_ << "assertion '" << #condition << "' failed"; \
            sre::Log::sreAssert(LOG_LOCATION, _sre_log_assert_msg_.str());     \
        }                                                                      \
    }
#endif
