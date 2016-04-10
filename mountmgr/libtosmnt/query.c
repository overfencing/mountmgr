
#include <string.h>
#include <errno.h>

#include <jansson.h>
#include <tosmnt.h>
#include <mountmgr/req_id.h>

#include "conn.h"


int tosmnt_query_drive(char drive_letter, struct tosmnt_info *info)
{
    int err_code = -1;
    json_t *arg = NULL, *ret = NULL;
    json_t *dev_name_json, *fs_name_json, *drive_letter_json;
    int _errno;

    arg = json_object();
    if(arg == NULL)
        goto end;
    json_object_set_new(arg, "drive_letter", json_integer(drive_letter));

    ret = send_request(REQ_ID_QUERY_DRIVE, arg, &err_code);
    if(ret == NULL || err_code != 0)
        goto end;

    dev_name_json = json_object_get(ret, "dev_name");
    if(dev_name_json == NULL || !json_is_string(dev_name_json)) {
        err_code = -1;
        goto end;
    }

    fs_name_json = json_object_get(ret, "fs_name");
    if(fs_name_json == NULL || !json_is_string(fs_name_json)) {
        err_code = -1;
        goto end;
    }

    drive_letter_json = json_object_get(ret, "drive_letter");
    if(drive_letter_json == NULL || !json_is_integer(drive_letter_json)) {
        err_code = -1;
        goto end;
    }

    if(info != NULL) {
        strlcpy(info->dev_name, json_string_value(dev_name_json), sizeof(info->dev_name));
        strlcpy(info->fs_name, json_string_value(fs_name_json), sizeof(info->fs_name));
        info->drive_letter = (char)json_integer_value(drive_letter_json);
    }

end:
    _errno = errno;
    if(ret)
        json_decref(ret);
    if(arg)
        json_decref(arg);
    errno = _errno;

    return err_code;
}

