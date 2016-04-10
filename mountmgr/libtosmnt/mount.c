
#include <errno.h>

#include <jansson.h>
#include <tosmnt.h>
#include <mountmgr/req_id.h>

#include "conn.h"


int tosmnt_unmount_drive(char drive_letter)
{
    int err_code = -1;
    json_t *arg = NULL, *ret = NULL;
    int _errno;

    arg = json_object();
    if(arg == NULL)
        goto end;
    json_object_set_new(arg, "drive_letter", json_integer(drive_letter));

    ret = send_request(REQ_ID_UNMOUNT_DRIVE, arg, &err_code);
    if(ret == NULL || err_code != 0)
        goto end;

end:
    _errno = errno;
    if(ret)
        json_decref(ret);
    if(arg)
        json_decref(arg);
    errno = _errno;

    return err_code;
}

