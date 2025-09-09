#pragma once
#include "uevr/API.hpp"
#include "Globals.hpp"
#include <cstdarg>
#include <cstdio>
#include <string>
#include <codecvt>
#include <locale>

namespace Logger {
    void DebugPrint(const char* Format, ...);
    void DebugPrint(const std::wstring& wstr);
}