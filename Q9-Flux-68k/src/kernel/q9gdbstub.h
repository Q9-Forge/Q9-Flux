#ifndef Q9_GDBSTUB_H
#define Q9_GDBSTUB_H

/* Optionaler GDB Remote Serial Protocol Server (nur localhost). */
int q9_gdbstub_init(void);
void q9_gdbstub_poll(int target_stopped);
void q9_gdbstub_close(void);
int q9_gdbstub_active(void);
int q9_gdbstub_connected(void);

#endif
