#ifndef QE_PLATFORM_H
#define QE_PLATFORM_H

#include <stdarg.h>

/* Shared key codes. Printable bytes and control characters retain their
 * terminal byte value; special keys start outside the byte range. */
enum qe_key_action {
    KEY_NULL = 0,
    CTRL_C = 3,
    CTRL_D = 4,
    CTRL_F = 6,
    CTRL_H = 8,
    TAB = 9,
    CTRL_L = 12,
    ENTER = 13,
    CTRL_Q = 17,
    CTRL_S = 19,
    CTRL_U = 21,
    ESC = 27,
    BACKSPACE = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

#define QE_STDIN 0
#define QE_STDOUT 1

int qe_term_enable_raw(int fd);
void qe_term_disable_raw(int fd);
int qe_term_read_key(int fd);
int qe_term_get_size(int ifd, int ofd, int *rows, int *cols);
int qe_term_write(int fd, const char *data, int length);
int qe_snprintf(char *buffer, int size, const char *format, ...);
int qe_vsnprintf(char *buffer, int size, const char *format, va_list arguments);

#endif
