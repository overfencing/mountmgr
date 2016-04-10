
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>

#include "log.h"


log_color_t g_log_color_none          = {NULL};

log_color_t g_log_color_dark_black    = {"0;30"};
log_color_t g_log_color_dark_red      = {"0;31"};
log_color_t g_log_color_dark_green    = {"0;32"};
log_color_t g_log_color_dark_yellow   = {"0;33"};
log_color_t g_log_color_dark_blue     = {"0;34"};
log_color_t g_log_color_dark_magenta  = {"0;35"};
log_color_t g_log_color_dark_cyan     = {"0;36"};
log_color_t g_log_color_dark_white    = {"0;37"};

log_color_t g_log_color_light_black   = {"1;30"};
log_color_t g_log_color_light_red     = {"1;31"};
log_color_t g_log_color_light_green   = {"1;32"};
log_color_t g_log_color_light_yellow  = {"1;33"};
log_color_t g_log_color_light_blue    = {"1;34"};
log_color_t g_log_color_light_magenta = {"1;35"};
log_color_t g_log_color_light_cyan    = {"1;36"};
log_color_t g_log_color_light_white   = {"1;37"};

log_level_t g_log_level_info          = {"INFO"};
log_level_t g_log_level_error         = {"ERROR"};
log_level_t g_log_level_fatal         = {"FATAL"};


static size_t get_timestamp(char *str, size_t size)
{
    time_t t;
    struct tm tm;

    time(&t);
    localtime_r(&t, &tm);

    return strftime(str, size, "%Y-%m-%d %a %H:%M:%S", &tm);
}

static size_t snprintf_color(char *str, size_t size, log_color_t log_color, const char *format, ...)
{
    int n;
    va_list valist;

    va_start(valist, format);

    if(log_color.log_color != LOG_COLOR_NONE.log_color) {
        static const char csi_format[] = "\e[%sm%s\e[0m";
        char colored_format[sizeof(csi_format) - 4 + strlen(log_color.log_color) + strlen(format)];

        sprintf(colored_format, csi_format, log_color.log_color, format);

        n = vsnprintf(str, size, colored_format, valist);
    }
    else
        n = vsnprintf(str, size, format, valist);

    va_end(valist);

    return (n >= 0 && n < size) ? n : 0;
}


void init_log()
{
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
}

void print_log(
    FILE *stream,
    log_color_t log_level_color,
    log_level_t log_level,
    const char *log_src_location,
    const char *log_src_function,
    log_color_t format_color,
    const char *format,
    ...
)
{
    char log[0x1000], buf[0x400] = "", *p = log;
    size_t size = sizeof(log), n;
    va_list valist;

    if(!isatty(fileno(stream))) {
        log_level_color = LOG_COLOR_NONE;
        format_color = LOG_COLOR_NONE;
    }

    n = get_timestamp(p, size);
    p += n;
    size -= n;

    if(log_src_location != NULL && log_src_function != NULL)
        snprintf(buf, sizeof(buf), "%s in %s():\n\t", log_src_location, log_src_function);
    else if(log_src_location != NULL)
        snprintf(buf, sizeof(buf), "%s:\n\t", log_src_location);
    else if(log_src_function != NULL)
        snprintf(buf, sizeof(buf), "%s(): ", log_src_function);

    n = snprintf_color(p, size, log_level_color, " %-5s %s", log_level.log_level, buf);
    p += n;
    size -= n;

    snprintf_color(p, size, format_color, "%s\n", format);

    va_start(valist, format);
    vfprintf(stream, log, valist);
    va_end(valist);
}

