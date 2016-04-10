
#include <stdlib.h>
#include <pthread.h>
#include <glob.h>
#include <sys/types.h>

#include "drive.h"
#include "drivemgr.h"
#include "log.h"


#define DRIVE_LETTER_FIRST 'd'
#define DRIVE_LETTER_LAST  'z'
#define DRIVE_LETTER_N     (DRIVE_LETTER_LAST - DRIVE_LETTER_FIRST + 1)


static struct drive *g_drive_list[DRIVE_LETTER_N];
static size_t g_drive_list_n = 0;
static pthread_mutex_t g_drive_list_lock = PTHREAD_MUTEX_INITIALIZER;


int is_valid_drive_letter(char drive_letter)
{
    return (drive_letter >= DRIVE_LETTER_FIRST && drive_letter <= DRIVE_LETTER_LAST);
}


struct drive *get_drive_from_list(char drive_letter)
{
    int index = drive_letter - DRIVE_LETTER_FIRST;
    struct drive *drive;

    pthread_mutex_lock(&g_drive_list_lock);

    drive = g_drive_list[index];
    if(drive != NULL)
        drive->ops->incref(drive);

    pthread_mutex_unlock(&g_drive_list_lock);

    return drive;
}


char acquire_drive_letter()
{
    int index;

    pthread_mutex_lock(&g_drive_list_lock);

    for(index = 0; index < DRIVE_LETTER_N; ++index)
        if(g_drive_list[index] == NULL)
            return (char)(index + DRIVE_LETTER_FIRST);

    rollback_drive_letter();
    return 0;
}

void commit_drive_letter(struct drive *drive)
{
    int index = drive->drive_letter - DRIVE_LETTER_FIRST;

    drive->ops->incref(drive);

    g_drive_list[index] = drive;
    ++g_drive_list_n;

    pthread_mutex_unlock(&g_drive_list_lock);
}

void rollback_drive_letter()
{
    pthread_mutex_unlock(&g_drive_list_lock);
}


void release_drive_letter_nolock(struct drive *drive)
{
    int index = drive->drive_letter - DRIVE_LETTER_FIRST;

    drive->ops->decref(drive);

    g_drive_list[index] = NULL;
    --g_drive_list_n;
}


int for_each_drive(int (*callback)(struct drive *, void *), void *data)
{
    int rc = 0;
    int index;

    pthread_mutex_lock(&g_drive_list_lock);

    for(index = 0; index < DRIVE_LETTER_N; ++index) {
        struct drive *drive = g_drive_list[index];

        if(drive != NULL) {
            rc = callback(drive, data);
            if(rc != 0)
                break;
        }
    }

    pthread_mutex_unlock(&g_drive_list_lock);

    return rc;
}


void init_drive_manager()
{
    static const char *pattern_list[] = {
        DEV_PATH "ad?",
        DEV_PATH "ada?",
        DEV_PATH "da?",
        DEV_PATH "cd?",
    };

    int k;

    log_info(LOG_COLOR_CYAN, "Initializing drive manager");

    for(k = 0; k < sizeof(pattern_list) / sizeof(*pattern_list); ++k) {
        int i;
        glob_t globbuf;

        if(glob(pattern_list[k], 0, NULL, &globbuf))
            continue;

        for(i = 0; i < globbuf.gl_pathc; ++i) {
            const char *dev_name = globbuf.gl_pathv[i] + sizeof(DEV_PATH) - 1;

            run_mount_manager(dev_name);
        }

        globfree(&globbuf);
    }
}

