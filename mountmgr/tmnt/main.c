
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <tosmnt.h>


static void query_drive(char drive_letter)
{
    int rc;
    struct tosmnt_info info;

    rc = tosmnt_query_drive(drive_letter, &info);
    if(rc == -1)
        perror("tosmnt_query_drive()");
    else if(rc == 1)
        fprintf(stdout, "\tNot found\n");
    else
        fprintf(stdout,
            "\tdev_name: \"%s\"\n"
            "\tfs_name: \"%s\"\n"
            "\tdrive_letter: \"%c\"\n",
            info.dev_name, info.fs_name, info.drive_letter
        );
}

static void unmount_drive(char drive_letter)
{
    int rc;

    rc = tosmnt_unmount_drive(drive_letter);
    if(rc == -1)
        perror("tosmnt_unmount_drive()");
    else if(rc == 1)
        fprintf(stdout, "\tNot found\n");
    else
        fprintf(stdout, "\tUnmounted successfully\n");
}

enum mode
{
    MODE_DEFAULT,
    MODE_QUERY,
    MODE_UNMOUNT,
};

static void (*mode_handler[])(char) = {
    [MODE_QUERY  ] = query_drive,
    [MODE_UNMOUNT] = unmount_drive,
};


static void print_usage_and_exit(char *argv[])
{
    fprintf(stderr,
        "Usage: %s -quh drive_letter ...\n"
        "-q: query the given drives\n"
        "-u: unmount the given drives\n"
        "-h: help\n"
        , argv[0]
    );
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int opt;
    enum mode mode = MODE_DEFAULT;
    size_t arg_n;
    char **arg;
    int i;

    opterr = 0;
    while((opt = getopt(argc, argv, "quh")) != -1)
        switch(opt) {
        case 'q': mode = MODE_QUERY; break;
        case 'u': mode = MODE_UNMOUNT; break;
        case 'h':
        default:
            print_usage_and_exit(argv);
        }

    arg_n = argc - optind;
    arg = &argv[optind];

    if(arg_n == 0 || mode == MODE_DEFAULT)
        print_usage_and_exit(argv);

    for(i = 0; i < arg_n; ++i) {
        char drive_letter = arg[i][0];

        fprintf(stdout, "%c:\n", drive_letter);
        mode_handler[mode](drive_letter);
    }

    exit(EXIT_SUCCESS);
}

