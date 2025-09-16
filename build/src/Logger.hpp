#pragma once
#include "uevr/API.hpp"
#include "Globals.hpp"
#include <cstdarg>
#include <codecvt>



#define PLUGIN_LOG_ONCE(...) { \
    static bool _logged_ = false; \
    if (!_logged_) { \
        _logged_ = true; \
        uevr::API::get()->log_info(__VA_ARGS__); \
    } }

#define PLUGIN_LOG_ONCE_ERROR(...) { \
    static bool _logged_ = false; \
    if (!_logged_) { \
        _logged_ = true; \
        uevr::API::get()->log_error(__VA_ARGS__); \
    } }

namespace Logger {
    void DebugPrint(const char* Format, ...);
    void DebugPrint(const std::wstring& wstr);
}