
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/event.h>

#include <jansson.h>
#include <mountmgr/common.h>
#include <mountmgr/req_id.h>

#include "drive.h"
#include "drivemgr.h"
#include "event.h"
#include "event_ipc.h"
#include "listener.h"
#include "log.h"


static int get_drive_using_letter(const json_t *arg, struct drive **drive)
{
    json_t *json;
    char drive_letter;
    struct drive *drive_tmp;

    json = json_object_get(arg, "drive_letter");
    if(json == NULL || !json_is_integer(json))
        return -1;
    drive_letter = (char)json_integer_value(json);

    if(!is_valid_drive_letter(drive_letter))
        return -1;

    drive_tmp = get_drive_from_list(drive_letter);
    if(drive_tmp == NULL)
        return 1;

    *drive = drive_tmp;

    return 0;
}


static int req_handler_query_drive(const json_t *arg, json_t *ret)
{
    int err_code;
    struct drive *drive;

    err_code = get_drive_using_letter(arg, &drive);
    if(err_code != 0)
        return err_code;

    json_object_set_new(ret, "dev_name", json_string(drive->dev_name));
    json_object_set_new(ret, "fs_name", json_string(drive->fs_name));
    json_object_set_new(ret, "drive_letter", json_integer(drive->drive_letter));

    drive->ops->decref(drive);

    return 0;
}

static int req_handler_unmount_drive(const json_t *arg, json_t *ret)
{
    int err_code;
    struct drive *drive;

    err_code = get_drive_using_letter(arg, &drive);
    if(err_code != 0)
        return err_code;

    drive->ops->unmount(drive);
    drive->ops->decref(drive);

    return 0;
}


typedef int (*req_handler_t)(const json_t *, json_t *);
static const req_handler_t g_req_handler[] = {
    [REQ_ID_QUERY_DRIVE  ] = req_handler_query_drive,
    [REQ_ID_UNMOUNT_DRIVE] = req_handler_unmount_drive,
};


static void send_response(int sock, int err_code, const json_t *ret)
{
    char *buf = NULL;
    size_t len;
    json_t *res = NULL;

    res = json_object();
    if(res == NULL)
        goto end;
    json_object_set_new(res, "err_code", json_integer(err_code));
    json_object_set(res, "ret", (json_t *)ret);

    buf = json_dumps(res, JSON_COMPACT | JSON_ENSURE_ASCII);
    if(buf == NULL)
        goto end;
    len = strlen(buf);

    if(send_full(sock, &len, sizeof(len)) != sizeof(len)) {
        log_error_errno();
        goto end;
    }

    if(send_full(sock, buf, len) != len) {
        log_error_errno();
        goto end;
    }

end:
    if(buf)
        free(buf);
    if(res)
        json_decref(res);
}

static void handle_request(int sock)
{
    char *buf = NULL;
    size_t len;
    json_t *req = NULL, *ret = NULL, *arg;
    json_error_t json_error;
    int req_id, err_code = -1;

    ret = json_object();
    if(ret == NULL)
        goto end;

    if(recv_full(sock, &len, sizeof(len)) == -1) {
        log_error_errno();
        goto end;
    }

    buf = (char *)malloc(len + 1);
    if(buf == NULL) {
        log_error_errno();
        goto end;
    }

    if(recv_full(sock, buf, len) == -1) {
        log_error_errno();
        goto end;
    }
    buf[len] = '\0';

    enable_listening_fd(sock);

    req = json_loads(buf, 0, &json_error);
    if(req == NULL) {
        log_error(LOG_COLOR_ERROR, "%d: %s", sock, json_error.text);
        goto end;
    }

    if(!json_is_object(req))
        goto invalid_request;

    arg = json_object_get(req, "req_id");
    if(arg == NULL || !json_is_integer(arg))
        goto invalid_request;
    req_id = json_integer_value(arg);

    if(req_id < 0 || req_id >= sizeof(g_req_handler) / sizeof(*g_req_handler))
        goto invalid_request;

    arg = json_object_get(req, "arg");
    if(arg == NULL || !json_is_object(arg))
        goto invalid_request;

    err_code = g_req_handler[req_id](arg, ret);

end:
    send_response(sock, err_code, ret);

    if(req)
        json_decref(req);
    if(buf)
        free(buf);
    if(ret)
        json_decref(ret);

    return;

invalid_request:
    log_error(LOG_COLOR_ERROR, "%d: Invalid request", sock);

    goto end;
}


struct event_ipc
{
    struct event event;

    int sock;
};


static int event_ipc_create(struct event *event, const struct kevent *ke)
{
    struct event_ipc *this = (struct event_ipc *)event;

    this->sock = ke->ident;

    return 0;
}

static void event_ipc_handle(struct event *event)
{
    struct event_ipc *this = (struct event_ipc *)event;

    handle_request(this->sock);
}


const struct event_ops event_ipc_ops = {
    .size = sizeof(struct event_ipc),
    .create = event_ipc_create,
    .destroy = destroy_event,
    .handle = event_ipc_handle,
};

