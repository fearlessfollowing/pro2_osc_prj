#ifndef _INSTA360_ARLOG_H
#define _INSTA360_ARLOG_H
#include <stdio.h>
#include <stdbool.h>
#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif


void arlog_configure(bool sendToLogcat, bool sendToFile, const char *logFilePath, bool redirectStd);

int __arlog_log_vprint(int prio, const char *tag, const char *fmt, va_list ap);

int __arlog_log_print(int prio, const char *tag,  const char *fmt, ...);

void arlog_close();

#ifdef __cplusplus
}
#endif

#endif