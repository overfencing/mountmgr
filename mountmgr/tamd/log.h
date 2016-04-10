
#ifndef __LOG_H__
#define __LOG_H__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct { const char *log_color; } log_color_t;

extern log_color_t g_log_color_none;

extern log_color_t g_log_color_dark_black;
extern log_color_t g_log_color_dark_red;
extern log_color_t g_log_color_dark_green;
extern log_color_t g_log_color_dark_yellow;
extern log_color_t g_log_color_dark_blue;
extern log_color_t g_log_color_dark_magenta;
extern log_color_t g_log_color_dark_cyan;
extern log_color_t g_log_color_dark_white;

extern log_color_t g_log_color_light_black;
extern log_color_t g_log_color_light_red;
extern log_color_t g_log_color_light_green;
extern log_color_t g_log_color_light_yellow;
extern log_color_t g_log_color_light_blue;
extern log_color_t g_log_color_light_magenta;
extern log_color_t g_log_color_light_cyan;
extern log_color_t g_log_color_light_white;

#define LOG_COLOR_NONE         g_log_color_none

#define LOG_COLOR_DARKBLACK    g_log_color_dark_black
#define LOG_COLOR_DARKRED      g_log_color_dark_red
#define LOG_COLOR_DARKGREEN    g_log_color_dark_green
#define LOG_COLOR_DARKYELLOW   g_log_color_dark_yellow
#define LOG_COLOR_DARKBLUE     g_log_color_dark_blue
#define LOG_COLOR_DARKMAGENTA  g_log_color_dark_magenta
#define LOG_COLOR_DARKCYAN     g_log_color_dark_cyan
#define LOG_COLOR_DARKWHITE    g_log_color_dark_white

#define LOG_COLOR_LIGHTBLACK   g_log_color_light_black
#define LOG_COLOR_LIGHTRED     g_log_color_light_red
#define LOG_COLOR_LIGHTGREEN   g_log_color_light_green
#define LOG_COLOR_LIGHTYELLOW  g_log_color_light_yellow
#define LOG_COLOR_LIGHTBLUE    g_log_color_light_blue
#define LOG_COLOR_LIGHTMAGENTA g_log_color_light_magenta
#define LOG_COLOR_LIGHTCYAN    g_log_color_light_cyan
#define LOG_COLOR_LIGHTWHITE   g_log_color_light_white

#define LOG_COLOR_DBLACK       LOG_COLOR_DARKBLACK
#define LOG_COLOR_DRED         LOG_COLOR_DARKRED
#define LOG_COLOR_DGREEN       LOG_COLOR_DARKGREEN
#define LOG_COLOR_DYELLOW      LOG_COLOR_DARKYELLOW
#define LOG_COLOR_DBLUE        LOG_COLOR_DARKBLUE
#define LOG_COLOR_DMAGENTA     LOG_COLOR_DARKMAGENTA
#define LOG_COLOR_DCYAN        LOG_COLOR_DARKCYAN
#define LOG_COLOR_DWHITE       LOG_COLOR_DARKWHITE

#define LOG_COLOR_LBLACK       LOG_COLOR_LIGHTBLACK
#define LOG_COLOR_LRED         LOG_COLOR_LIGHTRED
#define LOG_COLOR_LGREEN       LOG_COLOR_LIGHTGREEN
#define LOG_COLOR_LYELLOW      LOG_COLOR_LIGHTYELLOW
#define LOG_COLOR_LBLUE        LOG_COLOR_LIGHTBLUE
#define LOG_COLOR_LMAGENTA     LOG_COLOR_LIGHTMAGENTA
#define LOG_COLOR_LCYAN        LOG_COLOR_LIGHTCYAN
#define LOG_COLOR_LWHITE       LOG_COLOR_LIGHTWHITE

#define LOG_COLOR_BLACK        LOG_COLOR_LBLACK
#define LOG_COLOR_RED          LOG_COLOR_LRED
#define LOG_COLOR_GREEN        LOG_COLOR_LGREEN
#define LOG_COLOR_YELLOW       LOG_COLOR_LYELLOW
#define LOG_COLOR_BLUE         LOG_COLOR_LBLUE
#define LOG_COLOR_MAGENTA      LOG_COLOR_LMAGENTA
#define LOG_COLOR_CYAN         LOG_COLOR_LCYAN
#define LOG_COLOR_WHITE        LOG_COLOR_LWHITE

#define LOG_COLOR_INFO         LOG_COLOR_NONE
#define LOG_COLOR_ERROR        LOG_COLOR_MAGENTA
#define LOG_COLOR_FATAL        LOG_COLOR_RED


typedef struct { const char *log_level; } log_level_t;

extern log_level_t g_log_level_info;
extern log_level_t g_log_level_error;
extern log_level_t g_log_level_fatal;

#define LOG_LEVEL_INFO  g_log_level_info
#define LOG_LEVEL_ERROR g_log_level_error
#define LOG_LEVEL_FATAL g_log_level_fatal


#ifdef NDEBUG

#define LOG_SRC_LOCATION NULL

#else /* NDEBUG */

#define __TO_STRING(x) #x
#define _TO_STRING(x) __TO_STRING(x)
#define LOG_SRC_LOCATION __FILE__ ":" _TO_STRING(__LINE__)

#endif /* NDEBUG */

#define LOG_SRC_FUNCTION __func__


void init_log();

void print_log(
    FILE *stream,
    log_color_t log_level_color,
    log_level_t log_level,
    const char *log_src_location,
    const char *log_src_function,
    log_color_t format_color,
    const char *format,
    ...
);


#define log_info(log_color, format, ...)            \
    do {                                            \
        print_log(                                  \
            stdout,                                 \
            LOG_COLOR_INFO,                         \
            LOG_LEVEL_INFO,                         \
            NULL,                                   \
            NULL,                                   \
            log_color,                              \
            format,                                 \
            ##__VA_ARGS__                           \
        );                                          \
    } while(0)

#define log_error(log_color, format, ...)           \
    do {                                            \
        print_log(                                  \
            stderr,                                 \
            LOG_COLOR_ERROR,                        \
            LOG_LEVEL_ERROR,                        \
            LOG_SRC_LOCATION,                       \
            LOG_SRC_FUNCTION,                       \
            log_color,                              \
            format,                                 \
            ##__VA_ARGS__                           \
        );                                          \
    } while(0)

#define log_fatal(log_color, format, ...)           \
    do {                                            \
        print_log(                                  \
            stderr,                                 \
            LOG_COLOR_FATAL,                        \
            LOG_LEVEL_FATAL,                        \
            LOG_SRC_LOCATION,                       \
            LOG_SRC_FUNCTION,                       \
            log_color,                              \
            format,                                 \
            ##__VA_ARGS__                           \
        );                                          \
        abort();                                    \
    } while(0)

#define log_error_errno()                           \
    do {                                            \
        char _buf[0x100];                           \
        strerror_r(errno, _buf, sizeof(_buf));      \
        log_error(LOG_COLOR_ERROR, "%s", _buf);     \
    } while(0)

#define log_fatal_errno()                           \
    do {                                            \
        char _buf[0x100];                           \
        strerror_r(errno, _buf, sizeof(_buf));      \
        log_fatal(LOG_COLOR_FATAL, "%s", _buf);     \
    } while(0)


#ifdef __cplusplus
}
#endif


#endif /* __LOG_H__ */

