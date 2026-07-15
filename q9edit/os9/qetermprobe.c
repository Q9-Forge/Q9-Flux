/* q9edit OS-9 SCF/termcap platform probe. */

#include <stdio.h>
#include <string.h>

#include "qe_platform.h"

int main(int argc, char **argv)
{
    int rows;
    int columns;
    int keys[5];
    int index;

    (void)argc;
    (void)argv;

    if (qe_term_get_size(QE_STDIN, QE_STDOUT, &rows, &columns) == -1) {
        puts("qetermprobe: size failed");
        return 1;
    }
    printf("qetermprobe: %d columns, %d lines\n", columns, rows);

    if (qe_term_enable_raw(QE_STDIN) == -1) {
        puts("qetermprobe: raw enable failed");
        return 2;
    }
    qe_term_write(QE_STDOUT, "\033[32mqetermprobe: raw mode enabled\033[0m\r\n",
                  sizeof("\033[32mqetermprobe: raw mode enabled\033[0m\r\n") - 1);

    if (argc == 2 && strcmp(argv[1], "-k") == 0) {
        qe_term_write(QE_STDOUT, "qetermprobe: send up down left right q\r\n",
                      sizeof("qetermprobe: send up down left right q\r\n") - 1);
        for (index = 0; index < 5; index++) {
            keys[index] = qe_term_read_key(QE_STDIN);
        }
    }
    qe_term_disable_raw(QE_STDIN);
    puts("qetermprobe: terminal restored");

    if (argc == 2 && strcmp(argv[1], "-k") == 0) {
        printf("qetermprobe: keys %d %d %d %d %d\n",
               keys[0], keys[1], keys[2], keys[3], keys[4]);
    }
    return 0;
}
