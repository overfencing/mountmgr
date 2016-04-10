
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <jansson.h>
#include <mountmgr/common.h>
#include <mountmgr/req_id.h>

#include "conn.h"


static int g_tamd_sock = -1;
static pthread_mutex_t g_tamd_lock = PTHREAD_MUTEX_INITIALIZER;


static int open_tamd()
{
    int sock = -1;
    struct sockaddr_un addr;
    size_t len;
    int _errno;

    if(g_tamd_sock != -1)
        return g_tamd_sock;

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if(sock == -1)
        goto error;

    if(fcntl(sock, F_SETFD, FD_CLOEXEC) == -1)
        goto error;

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, SOCKFILE);
    len = sizeof(addr) - sizeof(addr.sun_path) + strlen(addr.sun_path) + 1;
    addr.sun_len = len;

    if(connect(sock, (struct sockaddr *)&addr, len) == -1)
        goto error;

    g_tamd_sock = sock;
    return g_tamd_sock;

error:
    _errno = errno;
    if(sock != -1)
        close(sock);
    errno = _errno;

    return -1;
}

static void close_tamd()
{
    if(g_tamd_sock == -1)
        return;

    close(g_tamd_sock);
    g_tamd_sock = -1;
}


static json_t *recv_response(int *err_code)
{
    json_t *ret = NULL;

    char *buf = NULL;
    size_t len;
    json_t *res = NULL, *err_code_json;
    int _errno;

    if(recv_full(g_tamd_sock, &len, sizeof(len)) == -1)
        goto error_before_unlock;

    buf = (char *)malloc(len + 1);
    if(buf == NULL)
        goto error_before_unlock;

    if(recv_full(g_tamd_sock, buf, len) == -1)
        goto error_before_unlock;
    buf[len] = '\0';

    pthread_mutex_unlock(&g_tamd_lock);

    res = json_loads(buf, 0, NULL);
    if(res == NULL)
        goto end;

    if(!json_is_object(res))
        goto end;

    err_code_json = json_object_get(res, "err_code");
    if(err_code_json == NULL || !json_is_integer(err_code_json))
        goto end;

    ret = json_object_get(res, "ret");
    if(ret == NULL || !json_is_object(ret)) {
        ret = NULL;
        goto end;
    }

    *err_code = json_integer_value(err_code_json);
    json_incref(ret);

end:
    _errno = errno;
    if(res)
        json_decref(res);
    if(buf)
        free(buf);
    errno = _errno;

    return ret;

error_before_unlock:
    _errno = errno;
    close_tamd();
    pthread_mutex_unlock(&g_tamd_lock);
    errno = _errno;

    goto end;
}

json_t *send_request(enum req_id req_id, const json_t *arg, int *err_code)
{
    json_t *ret = NULL;

    char *buf = NULL;
    size_t len;
    json_t *req = NULL;
    int _errno;

    req = json_object();
    if(req == NULL)
        goto end;
    json_object_set_new(req, "req_id", json_integer(req_id));
    json_object_set(req, "arg", (json_t *)arg);

    buf = json_dumps(req, JSON_COMPACT | JSON_ENSURE_ASCII);
    if(buf == NULL)
        goto end;
    len = strlen(buf);

    pthread_mutex_lock(&g_tamd_lock);

    if(open_tamd() == -1)
        goto error_before_unlock;

    if(send_full(g_tamd_sock, &len, sizeof(len)) != sizeof(len))
        goto error_before_unlock;

    if(send_full(g_tamd_sock, buf, len) != len)
        goto error_before_unlock;

    ret = recv_response(err_code);

end:
    _errno = errno;
    if(buf)
        free(buf);
    if(req)
        json_decref(req);
    errno = _errno;

    return ret;

error_before_unlock:
    _errno = errno;
    close_tamd();
    pthread_mutex_unlock(&g_tamd_lock);
    errno = _errno;

    goto end;
}

