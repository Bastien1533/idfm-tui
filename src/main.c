#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "tui/tui.h"
#include "tui/worker/worker.h"

static int get_opts(int argc, char **argv, struct worker_opts *opts)
{
    static const struct option long_args[] = {
        { "source", required_argument, NULL, 's' },
        { "dir", required_argument, NULL, 'd' },
        { NULL, 0, NULL, 0 }
    };
    int long_idx = 0;
    int opt;
    while ((opt = getopt_long(argc, argv, "sd:", long_args, &long_idx)) != -1)
    {
        if (opt == 'd')
            opts->gtfs_directory = optarg;
        else if (opt == 's')
            opts->gtfs_directory = optarg;
        else
            return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    struct worker_opts opts = { 0 };
    int stat = get_opts(argc, argv, &opts);
    render(&opts);
    return 0;
}
