
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/cdio.h>

#include "drive.h"
#include "drive_imp.h"
#include "log.h"


int is_ufs(const char *fs_name)
{
    return (!strcmp(fs_name, "freebsd-ufs"));
}

int is_fat(const char *fs_name)
{
    return (
        !strcmp(fs_name, "fat32") ||
        !strcmp(fs_name, "fat16") ||
        !strcmp(fs_name, "!12") ||
        !strcmp(fs_name, "vfat")
    );
}

int is_ntfs(const char *fs_name)
{
    return (!strcmp(fs_name, "ntfs"));
}

int is_iso9660(const char *fs_name)
{
    return (!strcmp(fs_name, "iso9660"));
}

int is_udf(const char *fs_name)
{
    return (!strcmp(fs_name, "udf"));
}

int is_known_fs(const char *fs_name)
{
    static const char *known_fs_name[] = {
        "GPT",
        "MBR",
        "freebsd",
        "freebsd-boot",
        "freebsd-swap",
    };

    int i;

    for(i = 0; i < sizeof(known_fs_name) / sizeof(*known_fs_name); ++i)
        if(!strcmp(fs_name, known_fs_name[i]))
            return !0;

    return 0;
}


static void unlock_cdrom(const char *dev_name)
{
    char dev_path[0x1000] = DEV_PATH;
    int fd;

    strlcat(dev_path, dev_name, sizeof(dev_path));
    fd = open(dev_path, O_WRONLY);
    if(fd == -1) {
        log_error_errno();
        return;
    }

    if(ioctl(fd, CDIOCALLOW) == -1)
        log_error_errno();

    close(fd);
}


struct drive_ufs
{
    struct drive drive;
};


static int drive_ufs_mount(struct drive *drive)
{
    struct drive_ufs *this = (struct drive_ufs *)drive;

    char command[0x1000];
    char *output;
    int exit_status;

    snprintf(command, sizeof(command), "mount -t ufs -o sync \"" DEV_PATH "%s\" \"" DRIVE_PATH "%c:\" 2>&1", this->drive.dev_name, this->drive.drive_letter);
    output = run_command(command, &exit_status);
    if(output == NULL)
        return -1;

    if(strstr(output, "Filesystem is not clean")) {
        snprintf(command, sizeof(command), "fsck -y -t ufs \"" DEV_PATH "%s\"", this->drive.dev_name);
        system(command);

        snprintf(command, sizeof(command), "mount -t ufs -o sync \"" DEV_PATH "%s\" \"" DRIVE_PATH "%c:\"", this->drive.dev_name, this->drive.drive_letter);
        exit_status = WEXITSTATUS(system(command));
    }

    free(output);

    if(exit_status != 0)
        return -1;

    return 0;
}


const struct drive_ops drive_ufs_ops = {
    .size = sizeof(struct drive_ufs),
    .create = NULL,
    .destroy = destroy_drive,
    .incref = incref_drive,
    .decref = decref_drive,
    .is_detached = is_detached_drive,
    .mount = drive_ufs_mount,
    .unmount = unmount_drive,
};


struct drive_fat
{
    struct drive drive;
};


static int drive_fat_mount(struct drive *drive)
{
    struct drive_fat *this = (struct drive_fat *)drive;

    char command[0x1000];

    snprintf(command, sizeof(command), "mount_msdosfs -L ko_KR.UTF-8 -o sync \"" DEV_PATH "%s\" \"" DRIVE_PATH "%c:\"", this->drive.dev_name, this->drive.drive_letter);
    if(WEXITSTATUS(system(command)))
        return -1;

    return 0;
}


const struct drive_ops drive_fat_ops = {
    .size = sizeof(struct drive_fat),
    .create = NULL,
    .destroy = destroy_drive,
    .incref = incref_drive,
    .decref = decref_drive,
    .is_detached = is_detached_drive,
    .mount = drive_fat_mount,
    .unmount = unmount_drive,
};


struct drive_ntfs
{
    struct drive drive;
};


static int drive_ntfs_mount(struct drive *drive)
{
    struct drive_ntfs *this = (struct drive_ntfs *)drive;

    char command[0x1000];

    snprintf(command, sizeof(command), "/usr/local/bin/ntfs-3g \"" DEV_PATH "%s\" \"" DRIVE_PATH "%c:\"", this->drive.dev_name, this->drive.drive_letter);
    if(WEXITSTATUS(system(command)))
        return -1;

    return 0;
}


const struct drive_ops drive_ntfs_ops = {
    .size = sizeof(struct drive_ntfs),
    .create = NULL,
    .destroy = destroy_drive,
    .incref = incref_drive,
    .decref = decref_drive,
    .is_detached = is_detached_drive,
    .mount = drive_ntfs_mount,
    .unmount = unmount_drive,
};


struct drive_iso9660
{
    struct drive drive;
};


static int drive_iso9660_is_detached(struct drive *drive)
{
    struct drive_iso9660 *this = (struct drive_iso9660 *)drive;

    char command[0x1000];

    snprintf(command, sizeof(command), "camcontrol tur '%s' >/dev/null", this->drive.dev_name);

    return !!WEXITSTATUS(system(command));
}

static int drive_iso9660_mount(struct drive *drive)
{
    struct drive_iso9660 *this = (struct drive_iso9660 *)drive;

    char command[0x1000];

    snprintf(command, sizeof(command), "mount_cd9660 \"" DEV_PATH "%s\" \"" DRIVE_PATH "%c:\"", this->drive.dev_name, this->drive.drive_letter);
    if(WEXITSTATUS(system(command)))
        return -1;

    unlock_cdrom(this->drive.dev_name);

    return 0;
}


const struct drive_ops drive_iso9660_ops = {
    .size = sizeof(struct drive_iso9660),
    .create = NULL,
    .destroy = destroy_drive,
    .incref = incref_drive,
    .decref = decref_drive,
    .is_detached = drive_iso9660_is_detached,
    .mount = drive_iso9660_mount,
    .unmount = unmount_drive,
};


struct drive_udf
{
    struct drive drive;
};


static int drive_udf_is_detached(struct drive *drive)
{
    return drive_iso9660_is_detached(drive);
}

static int drive_udf_mount(struct drive *drive)
{
    struct drive_udf *this = (struct drive_udf *)drive;

    char command[0x1000];

    snprintf(command, sizeof(command), "mount_udf \"" DEV_PATH "%s\" \"" DRIVE_PATH "%c:\"", this->drive.dev_name, this->drive.drive_letter);
    if(WEXITSTATUS(system(command)))
        return -1;

    unlock_cdrom(this->drive.dev_name);

    return 0;
}


const struct drive_ops drive_udf_ops = {
    .size = sizeof(struct drive_udf),
    .create = NULL,
    .destroy = destroy_drive,
    .incref = incref_drive,
    .decref = decref_drive,
    .is_detached = drive_udf_is_detached,
    .mount = drive_udf_mount,
    .unmount = unmount_drive,
};

