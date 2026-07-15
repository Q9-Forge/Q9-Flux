#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#endif

#include "qe_platform.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>

static struct termios original_termios;
static int raw_enabled;

int qe_term_enable_raw(int fd)
{
    struct termios raw;

    if (raw_enabled) return 0;
    if (!isatty(fd)) {
        errno = ENOTTY;
        return -1;
    }
    if (tcgetattr(fd, &original_termios) == -1) return -1;

    raw = original_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= CS8;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSAFLUSH, &raw) < 0) return -1;
    raw_enabled = 1;
    return 0;
}

void qe_term_disable_raw(int fd)
{
    if (raw_enabled) {
        tcsetattr(fd, TCSAFLUSH, &original_termios);
        raw_enabled = 0;
    }
}

int qe_term_read_key(int fd)
{
    int nread;
    char c;
    char seq[3];

    while ((nread = read(fd, &c, 1)) == 0) {
    }
    if (nread == -1) return -1;

    if (c != ESC) return (unsigned char)c;
    if (read(fd, seq, 1) == 0) return ESC;
    if (read(fd, seq + 1, 1) == 0) return ESC;

    if (seq[0] == '[') {
        if (seq[1] >= '0' && seq[1] <= '9') {
            if (read(fd, seq + 2, 1) == 0) return ESC;
            if (seq[2] == '~') {
                switch (seq[1]) {
                case '3': return DEL_KEY;
                case '5': return PAGE_UP;
                case '6': return PAGE_DOWN;
                }
            }
        } else {
            switch (seq[1]) {
            case 'A': return ARROW_UP;
            case 'B': return ARROW_DOWN;
            case 'C': return ARROW_RIGHT;
            case 'D': return ARROW_LEFT;
            case 'H': return HOME_KEY;
            case 'F': return END_KEY;
            }
        }
    } else if (seq[0] == 'O') {
        switch (seq[1]) {
        case 'H': return HOME_KEY;
        case 'F': return END_KEY;
        }
    }
    return ESC;
}

static int get_cursor_position(int ifd, int ofd, int *rows, int *cols)
{
    char buffer[32];
    unsigned int index;

    if (write(ofd, "\033[6n", 4) != 4) return -1;
    index = 0;
    while (index < sizeof(buffer) - 1) {
        if (read(ifd, buffer + index, 1) != 1) break;
        if (buffer[index] == 'R') break;
        index++;
    }
    buffer[index] = '\0';
    if (buffer[0] != ESC || buffer[1] != '[') return -1;
    if (sscanf(buffer + 2, "%d;%d", rows, cols) != 2) return -1;
    return 0;
}

int qe_term_get_size(int ifd, int ofd, int *rows, int *cols)
{
    struct winsize size;
    char sequence[32];
    int original_row;
    int original_col;

    if (ioctl(ofd, TIOCGWINSZ, &size) != -1 && size.ws_col != 0) {
        *cols = size.ws_col;
        *rows = size.ws_row;
        return 0;
    }
    if (get_cursor_position(ifd, ofd, &original_row, &original_col) == -1)
        return -1;
    if (write(ofd, "\033[999C\033[999B", 12) != 12) return -1;
    if (get_cursor_position(ifd, ofd, rows, cols) == -1) return -1;
    sprintf(sequence, "\033[%d;%dH", original_row, original_col);
    write(ofd, sequence, strlen(sequence));
    return 0;
}

int qe_term_write(int fd, const char *data, int length)
{
    return (int)write(fd, data, (size_t)length);
}

