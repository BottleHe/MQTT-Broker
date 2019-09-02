#ifndef __LOGGER_H__
#define __LOGGER_H__
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#ifndef LOG_TIME_FORMAT 
#define LOG_TIME_FORMAT "%F %T"
#endif

#ifdef __DEBUG_MODE
#define LOG_DEBUG_P(str) fprintf(stdout, str);
#define LOG_DEBUG(str) fprintf(stdout, "\033[0;36;1m[DEBUG]\033[0m \033[0;32;1m%s(%d)\033[0m: " str "\n", __FILE__, __LINE__)
#define LOG_DEBUG_T(str) do { \
    struct timeval tv; \
    gettimeofday(&tv, NULL); \
    struct tm* tm = localtime(&(tv.tv_sec)); \
    char timeStr[24] = {'\0'}; \
    strftime(timeStr, 23, LOG_TIME_FORMAT, tm); \
    fprintf(stdout, "\033[0;36;1m[DEBUG]\033[0m \033[0;37;1m%s.%d\033[0m [\033[0;32;1m%s:%d\033[0m]: " str "\n", timeStr, tv.tv_usec, __FILE__, __LINE__); \
} while (0)

#define LOG_DEBUG_EX_P(format, ...) fprintf(stdout, format, __VA_ARGS__)
#define LOG_DEBUG_EX(format, ...) fprintf(stdout, "\033[0;36;1m[DEBUG]\033[0m \033[0;32;1m%s(%d)\033[0m: " format "\n", __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG_EX_T(format, ...) do { \
    struct timeval tv; \
    gettimeofday(&tv, NULL); \
    struct tm* tm = localtime(&(tv.tv_sec)); \
    char timeStr[24] = {'\0'}; \
    strftime(timeStr, 23, LOG_TIME_FORMAT, tm); \
    fprintf(stdout, "\033[0;36;1m[DEBUG]\033[0m \033[0;37;1m%s.%d\033[0m [\033[0;32;1m%s:%d\033[0m]: " format "\n", timeStr, tv.tv_usec, __FILE__, __LINE__, __VA_ARGS__); \
} while (0)
#else

#define LOG_DEBUG_P(str)  
#define LOG_DEBUG(str)  
#define LOG_DEBUG_T(str)  
#define LOG_DEBUG_EX_P(format, ...) 
#define LOG_DEBUG_EX(format, ...)  
#define LOG_DEBUG_EX_T(format, ...)  

#endif

#define LOG_WRITE(format) fprintf(stdout, "\033[0;34;1m[LOG]\033[0m \033[0;32;1m%s(%d)\033[0m: " format "\n", __FILE__, __LINE__)
#define LOG_WRITE_T(format) do { \
    struct timeval tv; \
    gettimeofday(&tv, NULL); \
    struct tm* tm = localtime(&(tv.tv_sec)); \
    char timeStr[24] = {'\0'}; \
    strftime(timeStr, 23, LOG_TIME_FORMAT, tm); \
    fprintf(stdout, "\033[0;34;1m[LOG]\033[0m \033[0;37;1m%s.%d\033[0m [\033[0;32;1m%s:%d\033[0m]: " format "\n", timeStr, tv.tv_usec, __FILE__, __LINE__); \
} while (0)
#define LOG_WRITE_EX(format, ...) fprintf(stdout, "\033[0;34;1m[LOG]\033[0m \033[0;32;1m%s(%d)\033[0m: " format "\n", __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WRITE_EX_T(format, ...) do { \
    struct timeval tv; \
    gettimeofday(&tv, NULL); \
    struct tm* tm = localtime(&(tv.tv_sec)); \
    char timeStr[24] = {'\0'}; \
    strftime(timeStr, 23, LOG_TIME_FORMAT, tm); \
    fprintf(stdout, "\033[0;34;1m[LOG]\033[0m \033[0;37;1m%s.%d\033[0m [\033[0;32;1m%s:%d\033[0m]: " format "\n", timeStr, tv.tv_usec, __FILE__, __LINE__, __VA_ARGS__); \
} while (0)



#define LOG_ERROR(str) fprintf(stderr, "\033[0;31;1m[ERROR]\033[0m \033[0;32;1m%s(%d)\033[0m: " str "\n", __FILE__, __LINE__)
#define LOG_ERROR_T(str) do { \
    struct timeval tv; \
    gettimeofday(&tv, NULL); \
    struct tm* tm = localtime(&(tv.sec)); \
    char timeStr[24] = {'\0'}; \
    strftime(timeStr, 23, LOG_TIME_FORMAT, tm); \
    fprintf(stdout, "\033[0;31;1m[ERROR]\033[0m \033[0;37;1m%s.%d\033[0m [\033[0;32;1m%s:%d\033[0m]: " str "\n", timeStr, tv.tv_usec, __FILE__, __LINE__); \
} while (0)

#define LOG_ERROR_EX(format, ...) fprintf(stderr, "\033[0;31;1m[ERROR]\033[0m \033[0;32;1m%s(%d)\033[0m: " format "\n", __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR_EX_T(format, ...) do { \
    struct timeval tv; \
    gettimeofday(&tv, NULL); \
    struct tm* tm = localtime(&(tv_sec)); \
    char timeStr[24] = {'\0'}; \
    strftime(timeStr, 23, LOG_TIME_FORMAT, tm); \
    fprintf(stdout, "\033[0;31;1m[ERROR]\033[0m \033[0;37;1m%s.%d\033[0m [\033[0;32;1m%s:%d\033[0m]: " format "\n", timeStr, tv.tv_usec, __FILE__, __LINE__, __VA_ARGS__); \
} while (0)
#endif // __LOGGER_H__