#pragma once
#include "Logger.hpp"

#define MAX_DEBUG_MESSAGE_SIZE 2048

namespace {
    // Helper: convert wide string to UTF-8 std::string
    inline std::string WStringToUtf8(const std::wstring& wstr) {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        return conv.to_bytes(wstr);
    }
}

void Logger::DebugPrint(const char* Format, ...)
{
    if (!g_DebugEnabled)
        return;

    char FormattedMessage[MAX_DEBUG_MESSAGE_SIZE];
    va_list ArgPtr;
    va_start(ArgPtr, Format);
    vsnprintf(FormattedMessage, MAX_DEBUG_MESSAGE_SIZE, Format, ArgPtr);
    va_end(ArgPtr);

    // Print to console
    printf("%s\n", FormattedMessage);

    // Send to UE logger
    uevr::API::get()->log_info(FormattedMessage);
}

void Logger::DebugPrint(const std::wstring& wstr)
{
    if (!g_DebugEnabled)
        return;

    std::string utf8msg = WStringToUtf8(wstr);

    // Print to console
    printf("%s\n", utf8msg.c_str());

    // Send to UE logger
    uevr::API::get()->log_info(utf8msg.c_str());
}