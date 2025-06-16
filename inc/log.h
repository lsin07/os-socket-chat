#ifndef _ERROR_HANDLERS_H
#define _ERROR_HANDLERS_H

void sysErrExit(const char *msg);
void usrErrExit(const char *format, ...);
void dbgMsg(const char *file, const int line, const char *func, const char *format, ...);
void dbgWarn(const char *file, const int line, const char *func, const char *format, ...);
void dbgErrExit(const char *file, const int line, const char *func, const char *format, ...);
void usageErr(const char *format, ...);

#ifdef USE_DEBUG_LOGGING
#define DEBUG_MSG(...)      dbgMsg(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define DEBUG_WARN(...)     dbgWarn(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define DEBUG_EXIT(...)     dbgMsg(__FILE__, __LINE__, __func__, __VA_ARGS__)
#else
#define DEBUG_MSG(...)
#define DEBUG_WARN(...)
#define DEBUG_EXIT(...)
#endif

#endif