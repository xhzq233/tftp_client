#include <arpa/inet.h>
#include "export.h"
#include "logger/Logger.h"
#include <cstdlib>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "pkt/Packet.h"
#include <sys/stat.h>

// to create TCP/UDP socket for client
SOCKET socket_fd{};
// 本地recv的地址
sockaddr_in listen_addr{};
// 将要连接的服务器的地址
sockaddr_in server_addr{};
socklen_t server_addr_l = sizeof(server_addr);

std::stringstream msg;

#define  ZeroMemory(a) memset((&a), 0, sizeof((a)))

#define sendto_server(pkt) \
sendto(socket_fd, pkt.data, pkt.len, 0, (struct sockaddr *) &server_addr, sizeof(server_addr))

#define recvfrom_out(buf) \
recvfrom(socket_fd, (buf), TFTP_PACKET_MAX_SIZE, 0, (struct sockaddr *) &server_addr, &server_addr_l)

#define check_error \
if (IsError(recv_buf)) {\
    msg << "recv error, terminating.. Message: " << ErrorMessage(recv_buf);\
    logger.WriteError(msg);\
    fclose(file);   \
    callback_fn({.type=TF_TRANS_ERR, .value=TF_UNKNOWN_BLOCK_NO});\
    return SOCKET_RECV_ERROR;\
}

#define recv_check \
while (recv_size == -1 || !IsACK(recv_buf, block_no)) {\
    check_error    \
    callback_fn({.type=TF_TRANS_ERR, .value=block_no}); \
    msg << "recv wrong ack pkt, start to resend";\
    logger.WriteError(msg);\
    sendto_server(packet);\
    recv_size = recvfrom_out(recv_buf);\
}


#define socket_assert(x) \
if((x)== SOCKET_ERROR){  \
    logger.WriteError(msg<<":"<<__FILE__<<":"<<__FUNCTION__<<":"<<__LINE__<<":"); \
    perror("socket_assert");    \
    return SOCKET_ERROR;   \
}

long get_file_size(const char *filename) {
    struct stat stat_buf{};
    int rc = stat(filename, &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

int tf_init(const char *host, int port) {

    //服务器地址
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(host);
    server_addr.sin_port = htons(port);

    socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    socket_assert(socket_fd)
    int s;

    struct timeval timeout{};
    timeout.tv_sec = 2;
    s = setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(timeout));
    socket_assert(s)
    s = setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(timeout));
    socket_assert(s)

    // listen address
    bzero(&listen_addr, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    listen_addr.sin_port = htons(0); // any port

    auto bound = bind(socket_fd, (struct sockaddr *) &listen_addr, sizeof(listen_addr));
    socket_assert(bound)

    return FIN;
}


int tf_read(const char *filename, char mode, callback_fn_t *callback_fn) {
    Packet &packet = Packet::CreateRrq(filename, mode);
    char recv_buf[TFTP_PACKET_MAX_SIZE];
    ssize_t recv_size;
    size_t recv_data_len;
    FILE *file = nullptr;

    if (mode == TMOctet) {
        logger.WriteLog(msg << "open " << filename << " for OCTET tf_read");
        file = fopen(filename, "wb");
    } else if (mode == TMAscii) {
        logger.WriteLog(msg << "open " << filename << " for ASCII tf_read");
        file = fopen(filename, "w");
    }

    if (!file) {
        logger.WriteError(msg << "open " << filename << " failed");
        return OPENFILE_ERROR;
    }

    //request
    sendto_server(packet);

    // start data transfer
    short block_no = 1;
    while (true) {
        recv_size = recvfrom_out(recv_buf);

        // if recv error or recv unexpected pkt (is not data & block_no)
        // error handling
        while (recv_size == -1 || !IsData(recv_buf, block_no)) {
            check_error
            callback_fn({.type=TF_TRANS_ERR, .value=block_no});
            msg << "recv wrong data pkt, start to resend";
            logger.WriteError(msg);
            sendto_server(packet);
            recv_size = recvfrom_out(recv_buf);
        }

        if (block_no == 1) {
            callback_fn({.type=TF_START, .value=TF_UNKNOWN_BLOCK_NO});//-1 as TF_UNKNOWN_BLOCK_NO
        } else {
            callback_fn({.type=TF_PROGRESS, .value=block_no});
        }

        //received data
        auto data = TftpData(recv_buf);
        recv_data_len = recv_size - 4;// head 4 bytes
        fwrite(data, 1, recv_data_len, file);

        packet = Packet::CreateAck(block_no);
        sendto_server(packet);

        /*if last data packet then close*/
        if (recv_data_len < 512) {
            fclose(file);
            break;
        }
        block_no++;//next block
    }
    callback_fn({.type=TF_END, .value=block_no});
    return FIN;
}


int tf_write(const char *filename, char mode, callback_fn_t *callback_fn) {
    Packet &packet = Packet::CreateWrq(filename, mode);
    char recv_buf[TFTP_PACKET_MAX_SIZE];
    ZeroMemory(recv_buf);
    short block_no = 0;
    ssize_t recv_size;
    FILE *file = nullptr;

    if (mode == TMOctet) {
        logger.WriteLog(msg << "open " << filename << " for OCTET write");
        file = fopen(filename, "rb");
    } else if (mode == TMAscii) {
        logger.WriteLog(msg << "open " << filename << " for ASCII write");
        file = fopen(filename, "r");
    }
    if (!file) {
        logger.WriteError(msg << "open " << filename << " failed");
        return OPENFILE_ERROR;
    }

    // calculate total_block_no
    auto file_size = get_file_size(filename);
    if (file_size == -1) return OPENFILE_ERROR;
    auto total_block_no = (file_size % 512 <= 0) ? file_size / 512 : (file_size / 512) + 1;

    sendto_server(packet);
    recv_size = recvfrom_out(recv_buf);
    // if recv error or recv unexpected pkt (is not ack 0)
    recv_check
    callback_fn({.type=TF_START, .value=total_block_no});

    // start data transfer
    char send_buf[512];
    ZeroMemory(send_buf);
    size_t read_size;
    block_no++;
    while (true) {
        read_size = fread(send_buf, 1, 512, file);

        packet = Packet::CreateData(block_no, send_buf, read_size);

        sendto_server(packet);
        recv_size = recvfrom_out(recv_buf);
        recv_check

        // notify listeners that there is a packet transferred
        callback_fn({.type=TF_PROGRESS, .value=block_no});

        if (read_size < 512) {
            fclose(file);
            break;
        }
        block_no++;//next block
    }

    // notify listeners that there is a packet transferred
    callback_fn({.type=TF_END, .value=block_no});
    return FIN;
}