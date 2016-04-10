
#ifndef __LISTENER_H__
#define __LISTENER_H__


#include <stdio.h>


int is_devd_sock(int sock);
FILE *get_devd_fp();

void enable_listening_fd(int fd);

void add_timer(int ident, int msec, int first);
void init_listener();

void listener_main();


#endif /* __LISTENER_H__ */

