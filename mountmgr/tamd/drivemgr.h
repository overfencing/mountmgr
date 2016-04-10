
#ifndef __DRIVEMGR_H__
#define __DRIVEMGR_H__


#include "drive.h"


int is_valid_drive_letter(char drive_letter);

struct drive *get_drive_from_list(char drive_letter);

char acquire_drive_letter();
void commit_drive_letter(struct drive *drive);
void rollback_drive_letter();

void release_drive_letter_nolock(struct drive *drive);

int for_each_drive(int (*callback)(struct drive *, void *), void *data);

void init_drive_manager();


#endif /* __DRIVEMGR_H__ */

