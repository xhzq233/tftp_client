#ifndef TFTPMANAGER_H
#define TFTPMANAGER_H

typedef int SOCKET;
#define SOCKET_ERROR (-1)
#define SOCKET_RECV_ERROR (-2)
#define OPENFILE_ERROR (-4)
#define FIN (0)
#define TF_START (1)
#define TF_PROGRESS (2)
#define TF_TRANS_ERR (4)
#define TF_END (8)
#define TF_UNKNOWN_BLOCK_NO (-1)

#ifdef __APPLE__
#define EXPORT_HD extern "C" __attribute__((visibility("default"))) __attribute__((used))
#elif _WIN32
#define EXPORT_HD extern "C" __declspec(dllexport)
// remove annoying windows w_char
#undef UNICODE
#endif // __APPLE__

struct callback_data {
    const int type;
    const long value;
};

typedef void callback_fn_t(struct callback_data data);

EXPORT_HD int tf_read(const char *filename, char mode, callback_fn_t *callback_fn);

EXPORT_HD int tf_write(const char *filename, char mode, callback_fn_t *callback_fn);

EXPORT_HD int tf_init(const char *host, int port);

#endif
