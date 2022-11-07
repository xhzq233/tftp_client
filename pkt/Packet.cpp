//
// Created by 夏侯臻 on 06/11/2022.
//
#include "Packet.h"

Packet &Packet::CreateRrq(const char *filename, char mode) {
    static char data[TFTP_PACKET_MAX_SIZE] = {0, 1};
    static Packet res(data);
    res.len = 2;

    auto filename_len = strlen(filename) + 1;
    auto trans_mode = mode == TMAscii ? "netascii" : "octet";
    auto trans_mode_len = strlen(trans_mode) + 1;

    auto n_pos = data + 2;

    for (int i = 0; i < filename_len; ++i) *(n_pos++) = filename[i];

    n_pos = data + 2 + filename_len;

    for (int i = 0; i < trans_mode_len; ++i) *(n_pos++) = trans_mode[i];

    res.len += (trans_mode_len + filename_len);
    res.data = data;
    return res;
}

Packet &Packet::CreateWrq(const char *filename, char mode) {
    static char data[TFTP_PACKET_MAX_SIZE] = {0, 2};
    static Packet res(data);
    res.len = 2;

    auto filename_len = strlen(filename) + 1;
    auto trans_mode = mode == TMAscii ? "netascii" : "octet";
    auto trans_mode_len = strlen(trans_mode) + 1;

    auto n_pos = data + 2;

    for (int i = 0; i < filename_len; ++i) *(n_pos++) = filename[i];

    n_pos = data + 2 + filename_len;

    for (int i = 0; i < trans_mode_len; ++i) *(n_pos++) = trans_mode[i];

    res.len += (trans_mode_len + filename_len);

    return res;
}

Packet &Packet::CreateData(short block_no, const char *payload, size_t len) {
    static char data[TFTP_PACKET_MAX_SIZE] = {0, 3};
    static Packet res(data);
    res.len = 4;//op 2 bytes and block_no 2 bytes

//    *(short *) (res.data + 2) = block_no;//assign block_no into data
    *(res.data + 2) = (char) (block_no >> 8);
    *(res.data + 3) = (char) (block_no);

    res.len += len;//add payload_len

    auto n_pos = data + 4;
    for (int i = 0; i < len; ++i) {
        *(n_pos++) = payload[i];
    }
    return res;
}


Packet &Packet::CreateAck(short block_no) {
    static char data[TFTP_PACKET_MAX_SIZE] = {0, 4};
    static Packet res(data);
    res.len = 4;

    // 协议中是大端储存
    // assign block_no into data
//    *(short *) (res.data + 2) = block_no;
    *(res.data + 2) = (char) (block_no >> 8);
    *(res.data + 3) = (char) (block_no);

    return res;
}

Packet &Packet::CreateError(short code, const char *msg) {
    static char data[TFTP_PACKET_MAX_SIZE] = {0, 5};
    static Packet res(data);
    res.len = 4;

//    *(short *) (res.data + 2) = code;//assign code into data
    *(res.data + 2) = (char) (code >> 8);
    *(res.data + 3) = (char) (code);

    auto msg_len = strlen(msg);
    res.len += msg_len + 1;//add msg_len + 1(\0)

    auto n_pos = data + 4;

    for (int i = 0;; ++i) {
        *(n_pos++) = msg[i];
        if (msg[i] == '\0') break;
    }
    return res;
}

bool IsRRQ(const char *packet) {
    if (packet[1] == 1) return true;
    else return false;
}

bool IsWRQ(const char *packet) {
    if (packet[1] == 2) return true;
    else return false;
}

bool IsData(const char *packet, short block_no) {
    if (packet[1] == 3 &&
        *(packet + 2) == (char) (block_no >> 8) &&
        *(packet + 3) == (char) (block_no)
            )
        return true;
    else return false;
}

bool IsACK(const char *packet, short block_no) {
    if (packet[1] == 4 &&
        *(packet + 2) == (char) (block_no >> 8) &&
        *(packet + 3) == (char) (block_no)
            )
        return true;
    else return false;
}

bool IsError(const char *packet) {
    if (packet[1] == 5) return true;
    else return false;
}

const char *ErrorMessage(const char *packet) {
    return (packet + 4);
}

const char *TftpData(const char *packet) {
    return (packet + 4);
}