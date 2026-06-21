#include "SilberLogging.h"
#include <cstdio>
#include <cstdarg>

static void defaultErrorCallback(const char *message)
{
    std::fputs(message, stderr);
    std::fputc('\n', stderr);
}

static SilberErrorCallback g_errorCallback = defaultErrorCallback;

void setSilberErrorCallback(SilberErrorCallback callback)
{
    g_errorCallback = callback;
}

extern "C" void reportSilberError(const char *format, ...)
{
    if (!g_errorCallback) return;

    char buffer[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    g_errorCallback(buffer);
}
