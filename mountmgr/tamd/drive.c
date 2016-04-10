
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glob.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "drive.h"
#include "drive_imp.h"
#include "drivemgr.h"
#include "log.h"


char *run_command(const char *command, int *exit_status)
{
    char *output;
    size_t output_size;
    FILE *fout, *fin;
    char buf[0x1000];
    int status;

    fout = open_memstream(&output, &output_size);
    if(fout == NULL)
        return NULL;

    fin = popen(command, "r");
    if(fin == NULL) {
        fclose(fout);
        free(output);
        return NULL;
    }

    while(fgets(buf, sizeof(buf), fin) != NULL)
        fputs(buf, fout);
    fputc('\0', fout);

    status = WEXITSTATUS(pclose(fin));
    if(exit_status != NULL)
        *exit_status = status;

    fclose(fout);

    return output;
}


static struct drive *create_drive(const char *dev_name, const char *fs_name)
{
    struct drive *drive = NULL;
    const struct drive_ops *ops;

    if(is_ufs(fs_name))
        ops = &drive_ufs_ops;
    else if(is_fat(fs_name))
        ops = &drive_fat_ops;
    else if(is_ntfs(fs_name))
        ops = &drive_ntfs_ops;
    else if(is_iso9660(fs_name))
        ops = &drive_iso9660_ops;
    else if(is_udf(fs_name))
        ops = &drive_udf_ops;
    else {
        if(!is_known_fs(fs_name))
            log_error(LOG_COLOR_ERROR, "Unknown filesystem: %s", fs_name);

        goto error;
    }

    drive = (struct drive *)malloc(ops->size);
    if(drive == NULL)
        goto error;

    memset(drive, 0, ops->size);

    drive->ops = ops;
    drive->refcnt = 1;

    drive->dev_name = strdup(dev_name);
    if(drive->dev_name == NULL)
        goto error;

    drive->fs_name = strdup(fs_name);
    if(drive->fs_name == NULL)
        goto error;

    if(drive->ops->create != NULL)
        if(drive->ops->create(drive, dev_name, fs_name) == -1)
            goto error;

    return drive;

error:
    if(drive)
        destroy_drive(drive);

    return NULL;
}

void destroy_drive(struct drive *drive)
{
    free(drive->fs_name);
    free(drive->dev_name);
    free(drive);
}

void incref_drive(struct drive *drive)
{
    (void)__sync_add_and_fetch(&drive->refcnt, 1);
}

void decref_drive(struct drive *drive)
{
    if(__sync_sub_and_fetch(&drive->refcnt, 1) == 0)
        drive->ops->destroy(drive);
}

int is_detached_drive(struct drive *drive)
{
    char dev_path[0x1000] = DEV_PATH;

    strlcat(dev_path, drive->dev_name, sizeof(dev_path));

    return (access(dev_path, F_OK) == -1);
}

void unmount_drive(struct drive *drive)
{
    char command[0x1000];

    snprintf(command, sizeof(command), "umount -f \"" DRIVE_PATH "%c:\"", drive->drive_letter);
    system(command);
}


static char *get_fs_name_using_gpart(const char *dev_name)
{
    char *fs_name = NULL;

    char command[0x1000];
    char *output = NULL;
    int exit_status;
    char *p;

    snprintf(command, sizeof(command), "gpart show -p | grep '\\b%s\\b'", dev_name);
    output = run_command(command, &exit_status);
    if(output == NULL)
        log_fatal_errno();

    if(exit_status != 0)
        goto end;

    p = strstr(output, dev_name);
    if(p == NULL)
        goto end;

    p += strlen(dev_name) + 2;
    fs_name = p;

    for(; *p != ' ' && *p != '\n' && *p != '\0'; p++);
    *p = '\0';

    fs_name = strdup(fs_name);

end:
    if(output)
        free(output);

    return fs_name;
}

static char *get_fs_name_using_blkid(const char *dev_path)
{
    char command[0x1000];
    char *output = NULL;
    int exit_status;
    char *p;

    snprintf(command, sizeof(command), "/usr/local/sbin/blkid -o value -s TYPE '%s'", dev_path);
    output = run_command(command, &exit_status);
    if(output == NULL)
        log_fatal_errno();

    if(exit_status != 0)
        goto error;

    for(p = output; *p != '\n' && *p != '\0'; p++);
    *p = '\0';

    return output;

error:
    free(output);

    return NULL;
}

static char *get_fs_name(const char *dev_path, const char *dev_name)
{
    char *fs_name;

    fs_name = get_fs_name_using_gpart(dev_name);
    if(fs_name != NULL)
        return fs_name;

    fs_name = get_fs_name_using_blkid(dev_path);
    if(fs_name != NULL)
        return fs_name;

    log_error(LOG_COLOR_ERROR, "Cannot determine the filesystem: %s", dev_name);

    return NULL;
}

static int make_mountpoint(char drive_letter)
{
    char command[0x1000];

    snprintf(command, sizeof(command), "mkdir -m 0777 -p \"" DRIVE_PATH "%c:\"", drive_letter);
    if(WEXITSTATUS(system(command)))
        return -1;

    snprintf(command, sizeof(command), "chmod 0777 \"" DRIVE_PATH "%c:\"", drive_letter);
    if(WEXITSTATUS(system(command)))
        return -1;

    return 0;
}

static int remove_mountpoint(char drive_letter)
{
    char command[0x1000];

    snprintf(command, sizeof(command), "rmdir \"" DRIVE_PATH "%c:\"", drive_letter);
    if(WEXITSTATUS(system(command)))
        return -1;

    return 0;
}

static void update_drive_path()
{
    char command[0x1000];

    snprintf(command, sizeof(command), "touch \"" DRIVE_PATH "\"");
    system(command);
}

static int mount_drive(struct drive *drive)
{
    char drive_letter;

    drive_letter = acquire_drive_letter();
    if(drive_letter == 0) {
        log_error(LOG_COLOR_ERROR, "There are no available drive letters");

        return -1;
    }

    drive->drive_letter = drive_letter;
    if(make_mountpoint(drive->drive_letter) == -1)
        goto error;

    if(drive->ops->mount(drive) == -1)
        goto error;

    commit_drive_letter(drive);

    update_drive_path();

    log_info(LOG_COLOR_GREEN, "%s (%s) mounted on %c:", drive->dev_name, drive->fs_name, drive->drive_letter);

    return 0;

error:
    log_error(LOG_COLOR_ERROR, "Failed to mount %s (%s) on %c:", drive->dev_name, drive->fs_name, drive->drive_letter);

    remove_mountpoint(drive_letter);
    rollback_drive_letter();

    return -1;
}

static int is_symlink(const char *dev_path)
{
    struct stat st;

    return (lstat(dev_path, &st) == -1 || S_ISLNK(st.st_mode));
}

void run_mount_manager(const char *dev_name)
{
    char dev_path[0x1000] = DEV_PATH;
    glob_t globbuf;
    int i;

    strlcat(dev_path, dev_name, sizeof(dev_path));
    if(is_symlink(dev_path))
        return;

    strlcat(dev_path, "*", sizeof(dev_path));
    if(glob(dev_path, 0, NULL, &globbuf))
        return;

    for(i = 0; i < globbuf.gl_pathc; ++i) {
        const char *sub_dev_path = globbuf.gl_pathv[i];
        const char *sub_dev_name = sub_dev_path + sizeof(DEV_PATH) - 1;
        char *fs_name;
        struct drive *drive;

        if(is_symlink(sub_dev_path))
            continue;

        fs_name = get_fs_name(sub_dev_path, sub_dev_name);
        if(fs_name == NULL)
            continue;

        drive = create_drive(sub_dev_name, fs_name);
        free(fs_name);
        if(drive == NULL)
            continue;

        mount_drive(drive);

        drive->ops->decref(drive);
    }

    globfree(&globbuf);
}


static int check_detached(struct drive *drive, void *data)
{
    if(drive->ops->is_detached(drive)) {
        log_info(LOG_COLOR_YELLOW, "Detached: %s (%s)", drive->dev_name, drive->fs_name);

        drive->ops->unmount(drive);
    }

    return 0;
}

static int check_unmounted(struct drive *drive, void *data)
{
    char *output;
    char mountpoint[0x1000];

    output = run_command("mount", NULL);
    if(output == NULL)
        log_fatal_errno();

    snprintf(mountpoint, sizeof(mountpoint), " on " DRIVE_PATH "%c: ", drive->drive_letter);
    if(strstr(output, mountpoint) == NULL) {
        log_info(LOG_COLOR_YELLOW, "%s (%s) unmounted from %c:", drive->dev_name, drive->fs_name, drive->drive_letter);

        remove_mountpoint(drive->drive_letter);
        release_drive_letter_nolock(drive);
    }

    free(output);

    return 0;
}

void run_unmount_manager()
{
    for_each_drive(check_detached, NULL);
    for_each_drive(check_unmounted, NULL);
}

