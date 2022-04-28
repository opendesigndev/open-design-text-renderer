#pragma once

#include "fmt/core.h"

#include <functional>
#include <memory>
#include <string>

namespace textify {
namespace utils {

class Log
{
public:
    using FuncType = std::function<void(const std::string&)>;

    static std::unique_ptr<Log> createNoLog()
    {
        return std::make_unique<Log>(FuncType{}, FuncType{}, FuncType{});
    }

    Log(FuncType err, FuncType warn, FuncType info)
        : errFunc(err), warnFunc(warn), infoFunc(info)
    {
#ifdef TEXTIFY_DEBUG
        debugFunc = [] (const std::string& msg) { fmt::print("[DEBUG] {}\n", msg); };
#endif
    }

    template <typename... T>
    void error(const std::string& fmt, T&&... args) const
    {
        log(errFunc, fmt, args...);
    }

    template <typename... T>
    void warn(const std::string& fmt, T&&... args) const
    {
        log(warnFunc, fmt, args...);
    }

    template <typename... T>
    void info(const std::string& fmt, T&&... args) const
    {
        log(infoFunc, fmt, args...);
    }

    template <typename... T>
    void debug(const std::string& fmt, T&&... args) const
    {
#ifdef TEXTIFY_DEBUG
        log(debugFunc, fmt, args...);
#endif
    }

private:
    template <typename... T>
    void log(FuncType logFunc, const std::string& fmt, T&&... args) const
    {
        if (logFunc) {
            auto str = fmt::format(fmt, args...);
            logFunc(str);
        }
    }

    FuncType errFunc;
    FuncType warnFunc;
    FuncType infoFunc;
    FuncType debugFunc;
};

} // utils
} // textify
