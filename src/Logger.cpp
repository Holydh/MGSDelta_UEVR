#include "Logger.hpp"

#define MAX_DEBUG_MESSAGE_SIZE 2048

#include "ue/Types.hpp"

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
    uevr::API::get()->log_info("%s", FormattedMessage);
}

void Logger::DebugPrint(const std::wstring& wstr)
{
    if (!g_DebugEnabled)
        return;

    std::string utf8msg = ue::narrow(wstr);

    // Print to console
    printf("%s\n", utf8msg.c_str());

    // Send to UE logger
    uevr::API::get()->log_info("%s", utf8msg.c_str());
}