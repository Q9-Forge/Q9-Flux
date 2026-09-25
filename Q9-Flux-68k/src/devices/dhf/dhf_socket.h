/* dhf_socket.h - TCP Socket interface for remote DHF filesystem forwarding
 */

#ifndef DHF_SOCKET_H
#define DHF_SOCKET_H

#include <stdint.h>
#include <stddef.h>

#define DHF_NET_MAGIC 0x44484631 /* 'DHF1' */

#pragma pack(push, 1)
typedef struct dhf_net_header {
    uint32_t magic;
    uint8_t  command;
    uint8_t  status;
    uint32_t d0;        /* handle */
    uint32_t d1;        /* length / count / mode */
    uint32_t d2;        /* flags / whence */
    uint32_t path_len;  /* bytes of path string following header */
    uint32_t data_len;  /* bytes of data buffer following path */
} dhf_net_header_t;
#pragma pack(pop)

#ifdef __cplusplus
extern C {
#endif

/* Forward a command from emulator memory to remote server */
int dhf_socket_forward_cmd(int *cached_sock, const char *host, int port,
                           uint8_t cmd, uint32_t *d0, uint32_t *d1, uint32_t *d2,
                           const char *path,
                           const void *send_data, size_t send_len,
                           void *recv_data, size_t *recv_len,
                           uint8_t *out_status);

/* Run daemon server on remote host */
int dhf_socket_server_run(int port, const char *basepath, volatile int *stop_flag);

#ifdef __cplusplus
}
#endif

#endif /* DHF_SOCKET_H */
