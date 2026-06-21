#pragma once

typedef void (*SilberErrorCallback)(const char *message);

// Registers a custom callback function for error logging.
// Pass nullptr to completely suppress all Silber library prints.
void setSilberErrorCallback(SilberErrorCallback callback);

#ifdef __cplusplus
extern "C" {
#endif

// Internal helper for formatting and reporting errors inside Silber
void reportSilberError(const char *format, ...);

#ifdef __cplusplus
}
#endif
