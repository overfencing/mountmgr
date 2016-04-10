
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <mountmgr/common.h>


ssize_t recv_full(int fd, void *buf, size_t size)
{
    uint8_t *p = (uint8_t *)buf;

    while(size > 0) {
        ssize_t n;

        do n = recv(fd, p, size, 0);
        while(n == -1 && errno == EINTR);
        if(n > 0) {
            p += n;
            size -= n;
        }
        else if(n == 0)
            break;
        else
            return n;
    }

    return (p - (uint8_t *)buf);
}

ssize_t send_full(int fd, const void *buf, size_t size)
{
    const uint8_t *p = (const uint8_t *)buf;

    while(size > 0) {
        ssize_t n;

        do n = send(fd, p, size, 0);
        while(n == -1 && errno == EINTR);
        if(n > 0) {
            p += n;
            size -= n;
        }
        else if(n == 0)
            break;
        else
            return n;
    }

    return (p - (const uint8_t *)buf);
}

