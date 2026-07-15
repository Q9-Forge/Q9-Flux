/*
 * q9edit OS-9 toolchain probe.
 *
 * This file deliberately uses only ANSI C89 facilities.  Its purpose is to
 * prove the xcc compile/link path before editor or terminal code is added.
 */

#include <stdio.h>
#include <stdlib.h>
#include <termcap.h>

#define QE_TERMCAP_BUFFER_SIZE 2048

/* OS-9 program modules start with a small stack.  Large editor and termcap
 * buffers must not be automatic variables. */
static char termcap_buffer[QE_TERMCAP_BUFFER_SIZE];

int main(int argc, char **argv)
{
    char *term_name;
    int term_status;
    int columns;
    int lines;

    (void)argc;
    (void)argv;

    puts("qeprobe: xcc OS-9 build works");

    term_name = getenv("TERM");
    if (term_name == NULL || *term_name == '\0') {
        puts("qeprobe: TERM is not set");
        return 2;
    }

    term_status = tgetent(termcap_buffer, term_name);
    if (term_status != 1) {
        printf("qeprobe: termcap entry '%s' not found (%d)\n",
               term_name, term_status);
        return 3;
    }

    columns = tgetnum("co");
    lines = tgetnum("li");
    printf("qeprobe: termcap '%s', %d columns, %d lines\n",
           term_name, columns, lines);
    puts("\033[31mqeprobe: red ANSI color\033[0m");
    return 0;
}
