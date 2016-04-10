
#ifndef __DRIVE_H__
#define __DRIVE_H__


#include <stddef.h>


#define DEV_PATH   "/dev/"
#define DRIVE_PATH "/volume/"


struct drive;

struct drive_ops
{
    size_t size;

    int (*create)(struct drive *, const char *, const char *);
    void (*destroy)(struct drive *);
    void (*incref)(struct drive *);
    void (*decref)(struct drive *);
    int (*is_detached)(struct drive *);
    int (*mount)(struct drive *);
    void (*unmount)(struct drive *);
};

struct drive
{
    const struct drive_ops *ops;

    volatile size_t refcnt;

    char *dev_name;
    char *fs_name;
    char drive_letter;
};


char *run_command(const char *command, int *exit_status);

void destroy_drive(struct drive *drive);
void incref_drive(struct drive *drive);
void decref_drive(struct drive *drive);
int is_detached_drive(struct drive *drive);
void unmount_drive(struct drive *drive);

void run_mount_manager(const char *dev_name);

void run_unmount_manager();


#endif /* __DRIVE_H__ */

