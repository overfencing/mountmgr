
#ifndef __DRIVE_IMP_H__
#define __DRIVE_IMP_H__


#include "drive.h"


extern const struct drive_ops drive_ufs_ops;
extern const struct drive_ops drive_fat_ops;
extern const struct drive_ops drive_ntfs_ops;
extern const struct drive_ops drive_iso9660_ops;
extern const struct drive_ops drive_udf_ops;


int is_ufs(const char *fs_name);
int is_fat(const char *fs_name);
int is_ntfs(const char *fs_name);
int is_iso9660(const char *fs_name);
int is_udf(const char *fs_name);
int is_known_fs(const char *fs_name);


#endif /* __DRIVE_IMP_H__ */

