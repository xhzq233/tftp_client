#include "logger/Logger.h"
#include "export.h"
#include <chrono>
#include <sys/stat.h>
#include "pkt/Packet.h"

char host[50] = "";
char filename[1000] = "";
int port = 69;
char mode = TMOctet;

long gfs() {
    struct stat stat_buf{};
    int rc = stat(filename, &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

std::ostringstream log_stream;

long total_block = TF_UNKNOWN_BLOCK_NO;

void callback(callback_data data) {
    if (data.type == TF_START) {
        log_stream << "Start transfer, total block=" << data.value;
        logger.WriteLog(log_stream);
        total_block = data.value;
    } else if (data.type == TF_PROGRESS) {
        if (total_block != TF_UNKNOWN_BLOCK_NO) {
            log_stream << "Writing, now progress=" << (double) data.value / (double) total_block << ", blockno="
                       << data.value;
        } else {
            log_stream << "Reading, now blockno=" << data.value;
        }
        logger.WriteLog(log_stream);
    } else if (data.type == TF_END) {
        log_stream << "Transferred, total block=" << data.value;
        logger.WriteLog(log_stream);
    } else if (data.type == TF_TRANS_ERR) {
//        log_stream << "Error, now blockno=" << data.value;
//        logger.WriteError(log_stream);
    }
}

int main(int argc, char **argv) {
    using namespace std::chrono;
    long fsize;
    duration<double, std::milli> c{};
    high_resolution_clock::time_point stime;
    high_resolution_clock::time_point etime;

#define usage puts("\n*usage: ./tftp_client <host> [port=69] <filename> [mode=read] [proto=octet]\n");
    if (argc <= 2) {
        usage
        exit(-1);
    }
    strcpy(host, argv[1]);
    if (argc >= 4) {
        port = atoi(argv[2]);
        if (port > 65535 || port < 0) {
            printf("unexpected dst port");
            usage
            exit(-1);
        }
        strcpy(filename, argv[3]);
    } else if (argc == 3) {
        strcpy(filename, argv[2]);
    }

    bool read = true;
    if (argc >= 5) {
        if (strcmp(argv[4], "read") == 0) {

        } else if (strcmp(argv[4], "write") == 0) {
            read = false;
        } else {
            printf("unexpected r/w mode");
            usage
            exit(-1);
        }
    }

    if (argc == 6) {
        if (strcmp(argv[5], "octet") == 0) {

        } else if (strcmp(argv[5], "ascii") == 0) {
            mode = TMAscii;
        } else {
            printf("unexpected proto(octet/ascii)");
            usage
            exit(-1);
        }
    }

    if (tf_init(host, port) == -1) {
        perror("init error");
        usage
        return -1;
    }

    stime = high_resolution_clock::now();
    int res;
    if (read) {
        res = tf_read(filename, mode, callback);
    } else {
        res = tf_write(filename, mode, callback);
    }
    if (res != FIN) exit(-1);
    etime = high_resolution_clock::now();
    //end
    fsize = gfs();
    c = (etime - stime);
    printf("%.3lf Kb/s\n", ((double) fsize * 1000 / 1024) * 8 / c.count());
}
