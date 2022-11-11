
#ifdef __APPLE__

#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <cstdlib>

#elif _WIN32
#include <WS2tcpip.h>
#include <windows.h>
#define _CRT_SECURE_NO_WARNINGS
#define ssize_t long
#pragma comment(lib,"ws2_32.lib")
#endif

#include "export.h"
#include "logger/Logger.h"
#include <cstdio>
#include "pkt/Packet.h"
#include <sys/stat.h>

// to create TCP/UDP socket for client
SOCKET socket_fd{};
// 本地recv的地址
sockaddr_in listen_addr{};
// 建立连接的外部地址
sockaddr_in conn_addr{};
socklen_t conn_addr_l = sizeof(sockaddr_in);
// 信任地址 target
sockaddr_in server_addr{};

std::stringstream msg;

#undef ZeroMemory
#define  ZeroMemory(a) memset((&a), 0, sizeof((a)))

#define sendto_server(pkt) \
sendto(socket_fd, pkt.data, pkt.len, 0, (struct sockaddr *) &conn_addr, conn_addr_l)

#define recvfrom_out(buf) \
recvfrom(socket_fd, (buf), TFTP_PACKET_MAX_SIZE, 0, (struct sockaddr *) &conn_addr, &conn_addr_l)


#define check_error \
if (IsError(recv_buf)) {\
    msg << "recv error, terminating.. Message: " << ErrorMessage(recv_buf);\
    logger.WriteError(msg);\
    fclose(file);   \
    callback_fn({.type=TF_TRANS_ERR, .value=TF_UNKNOWN_BLOCK_NO});\
    return SOCKET_RECV_ERROR;\
}

#define check_origin \
if (memcmp(&conn_addr,&server_addr,sizeof(sockaddr_in))!=0) { \
    msg << "recv from "<< addrtos(conn_addr) <<" which is not same with target server, ignored";\
    logger.WriteError(msg);\
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

long get_file_size(const char *filename) {
    struct stat stat_buf{};
    int rc = stat(filename, &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

const char *addrtos(sockaddr_in addr) {
    static char buf[1000];
    sprintf(buf, "host %s, port %d", inet_ntoa(addr.sin_addr), htons(addr.sin_port));
    return buf;
}

int tf_init(const char *host, int port) {

    //服务器地址
    ZeroMemory(conn_addr);
    conn_addr.sin_family = AF_INET;
    conn_addr.sin_addr.s_addr = inet_addr(host);
    conn_addr.sin_port = htons(port);
    memcpy(&server_addr, &conn_addr, conn_addr_l);

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
    ZeroMemory(listen_addr);
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    listen_addr.sin_port = htons(23333); // any port

    s = bind(socket_fd, (struct sockaddr *) &listen_addr, sizeof(listen_addr));
    socket_assert(s)

    printf("bound server %s\n", addrtos(conn_addr));
    printf("listening at %s\n", addrtos(listen_addr));
    return FIN;
}


int tf_read(const char *filename, char mode, callback_fn_t *callback_fn) {
    Packet &packet = Packet::CreateRrq(filename, mode);
    char recv_buf[TFTP_PACKET_MAX_SIZE];
    ssize_t recv_size;
    size_t recv_data_len;
    FILE *file = nullptr;
    short block_no = 1;

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

    // Connection established successfully

    // start data transfer

    while (true) {
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

        recv_size = recvfrom_out(recv_buf);
        check_origin
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