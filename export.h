#ifndef TFTPMANAGER_H
#define TFTPMANAGER_H

#define SOCKET_ERROR (-1)
#define SOCKET_RECV_ERROR (-2)
#define OPENFILE_ERROR (-4)
#define RETRY_MAX_ERROR (-8)
#define FIN (0)
#define TF_START (1)
#define TF_PROGRESS (2)
#define TF_TRANS_ERR (4)
#define TF_END (8)
#define TF_UNKNOWN_BLOCK_NO (-1)

#ifdef __APPLE__
#define EXPORT_HD extern "C" __attribute__((visibility("default"))) __attribute__((used))
typedef int SOCKET;
#elif _WIN32
#define EXPORT_HD extern "C" __declspec(dllexport)
// remove annoying windows w_char
#undef UNICODE
#else
#define EXPORT_HD
typedef int SOCKET;
#endif // __APPLE__

struct callback_data {
    const int type;
    const long value;
};

typedef void callback_fn_t(struct callback_data data);

EXPORT_HD int tf_read(const char *filename, char mode, callback_fn_t *callback_fn);

EXPORT_HD int tf_write(const char *filename, char mode, callback_fn_t *callback_fn);

EXPORT_HD int tf_init(const char *host, int port);


#define socket_assert(x) \
if(((int)(x)) < 0){      \
    msg<<":"<<__FILE__<<":"<<__FUNCTION__<<":"<<__LINE__<<":";                     \
    logger.WriteError(msg); \
    perror("socket_assert");    \
    return SOCKET_ERROR;   \
}
#endif
