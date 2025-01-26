#ifndef _LOG_H
#define _LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

void loginfo(const char *fmt, ...);
void logerr(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif // _LOG_H
