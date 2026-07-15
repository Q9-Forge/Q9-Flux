#define _OPT_PROTOS 1

#include "qe_platform.h"

#include <modes.h>
#include <sgstat.h>
#include <sg_codes.h>
#include <signal.h>
#include <stdlib.h>
#include <termcap.h>

#define QE_TERMCAP_BUFFER_SIZE 2048

static struct sgbuf saved_options;
static int raw_enabled;
static char termcap_buffer[QE_TERMCAP_BUFFER_SIZE];

int qe_term_enable_raw(int fd)
{
    struct sgbuf raw;

    if (raw_enabled) return 0;
    if (_gs_opt(fd, &saved_options) == -1) return -1;

    raw = saved_options;
    raw.sg_case = 0;
    raw.sg_backsp = 0;
    raw.sg_delete = 0;
    raw.sg_echo = 0;
    raw.sg_alf = 0;
    raw.sg_pause = 0;
    raw.sg_bspch = 0;
    raw.sg_dlnch = 0;
    raw.sg_eorch = 0;
    raw.sg_eofch = 0;
    raw.sg_rlnch = 0;
    raw.sg_dulnch = 0;
    raw.sg_psch = 0;
    raw.sg_kbich = 0;
    raw.sg_kbach = 0;
    raw.sg_xon = 0;
    raw.sg_xoff = 0;

    if (_ss_opt(fd, &raw) == -1) return -1;
    raw_enabled = 1;
    return 0;
}

void qe_term_disable_raw(int fd)
{
    if (raw_enabled) {
        _ss_opt(fd, &saved_options);
        raw_enabled = 0;
    }
}

static int read_byte(int fd, char *value)
{
    int count;

    count = read(fd, value, 1);
    if (count == 1) return 1;
    if (count < 0) return -1;
    return 0;
}

static int read_optional_byte(int fd, char *value)
{
    int tries;

    /* /x1 may expose the bytes of one Telnet key over several scheduler
       passes. Keep ESC usable on its own, but allow an ANSI sequence time. */
    for (tries = 0; tries < 20; tries++) {
        if (_gs_rdy(fd) > 0) return read_byte(fd, value);
        tsleep(1);
    }
    return 0;
}

int qe_term_read_key(int fd)
{
    char c;
    char sequence[3];
    int result;

    do {
        result = read_byte(fd, &c);
    } while (result == 0);
    if (result < 0) return -1;
    if ((unsigned char)c != ESC) return (unsigned char)c;

    if (read_optional_byte(fd, sequence) != 1) return ESC;
    if (read_optional_byte(fd, sequence + 1) != 1) return ESC;

    if (sequence[0] == '[') {
        if (sequence[1] >= '0' && sequence[1] <= '9') {
            if (read_optional_byte(fd, sequence + 2) != 1) return ESC;
            if (sequence[2] == '~') {
                switch (sequence[1]) {
                case '3': return DEL_KEY;
                case '5': return PAGE_UP;
                case '6': return PAGE_DOWN;
                }
            }
        } else {
            switch (sequence[1]) {
            case 'A': return ARROW_UP;
            case 'B': return ARROW_DOWN;
            case 'C': return ARROW_RIGHT;
            case 'D': return ARROW_LEFT;
            case 'H': return HOME_KEY;
            case 'F': return END_KEY;
            }
        }
    } else if (sequence[0] == 'O') {
        switch (sequence[1]) {
        case 'H': return HOME_KEY;
        case 'F': return END_KEY;
        }
    }
    return ESC;
}

int qe_term_get_size(int ifd, int ofd, int *rows, int *cols)
{
    char *term_name;
    char reply[32];
    int status;
    int value;
    int length;
    int row_value;
    int col_value;
    int index;

    term_name = getenv("TERM");
    if (term_name == NULL || *term_name == '\0') term_name = "q9";

    status = tgetent(termcap_buffer, term_name);
    if (status == 1) {
        value = tgetnum("li");
        *rows = value > 0 ? value : 24;
        value = tgetnum("co");
        *cols = value > 0 ? value : 80;
    } else {
        *rows = 24;
        *cols = 80;
    }

    /* Ask the actual ANSI terminal as well. termcap describes the terminal
       type, but q9term's co/li values cannot follow a resized host window. */
    if (raw_enabled &&
        qe_term_write(ofd, "\033[999C\033[999B\033[6n", 16) == 16) {
        length = 0;
        while (length < (int)sizeof(reply) - 1) {
            if (read_optional_byte(ifd, reply + length) != 1) break;
            if (reply[length++] == 'R') break;
        }
        reply[length] = '\0';
        if (length >= 6 && reply[0] == ESC && reply[1] == '[') {
            row_value = 0;
            col_value = 0;
            index = 2;
            while (reply[index] >= '0' && reply[index] <= '9')
                row_value = row_value * 10 + reply[index++] - '0';
            if (reply[index++] == ';') {
                while (reply[index] >= '0' && reply[index] <= '9')
                    col_value = col_value * 10 + reply[index++] - '0';
                if (reply[index] == 'R' && row_value > 0 && col_value > 0) {
                    *rows = row_value;
                    *cols = col_value;
                }
            }
        }
    }
    return 0;
}

int qe_term_write(int fd, const char *data, int length)
{
    int total;
    int count;

    total = 0;
    while (total < length) {
        count = write(fd, data + total, length - total);
        if (count <= 0) return -1;
        total += count;
    }
    return total;
}
