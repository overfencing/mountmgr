
#include <string.h>
#include <unistd.h>

#include <jansson.h>

#include "conf.h"
#include "log.h"


#define CONF_KEY(key) g_key_##key
#define CONF_VAL(key) g_val_##key

#define DEFINE_CONF(key, type, default_value)           \
    static const char CONF_KEY(key)[] = #key;           \
    static type CONF_VAL(key) = default_value;          \
    DECLARE_CONF(key, type) { return CONF_VAL(key); }

DEFINE_CONF(worker_n, size_t, 4);
DEFINE_CONF(delay_unmount_msec, unsigned int, 2000);


#define DUMP_CONF(key, format)                  \
    log_info(LOG_COLOR_NONE, "%-20s = " format, \
        CONF_KEY(key), CONF_VAL(key)            \
    )

static void dump_conf()
{
    DUMP_CONF(worker_n, "%zu");
    DUMP_CONF(delay_unmount_msec, "%u");
}


#define LOAD_CONF_UINT(key)                         \
    else if(!strcmp(key_name, CONF_KEY(key))) {     \
        if(!json_is_integer(value))                 \
            goto syntax_error;                      \
                                                    \
        CONF_VAL(key) = json_integer_value(value);  \
                                                    \
        if(!(CONF_VAL(key) > 0))                    \
            goto must_be_greater_than_zero;         \
    }

void load_conf(const char *conf_file)
{
    json_t *json, *value;
    json_error_t json_error;
    const char *key_name;

    log_info(LOG_COLOR_CYAN, "Loading configuration from %s", conf_file);

    if(access(conf_file, F_OK) == -1) {
        log_error(LOG_COLOR_YELLOW, "%s not found, using default configuration", conf_file);
        goto end;
    }

    json = json_load_file(conf_file, JSON_REJECT_DUPLICATES | JSON_ALLOW_NUL, &json_error);
    if(json == NULL)
        log_fatal(LOG_COLOR_FATAL, "%s:%d:%d: %s", conf_file, json_error.line, json_error.column, json_error.text);

    if(!json_is_object(json))
        goto syntax_error;

    json_object_foreach(json, key_name, value) {
        if(0) {}
        LOAD_CONF_UINT(worker_n)
        LOAD_CONF_UINT(delay_unmount_msec)
        else
            goto syntax_error;
    }

end:
    dump_conf();
    return;

syntax_error:
    log_fatal(LOG_COLOR_FATAL, "%s: Syntax error", conf_file);

must_be_greater_than_zero:
    log_fatal(LOG_COLOR_FATAL, "%s: %s must be greater than zero", conf_file, key_name);
}

