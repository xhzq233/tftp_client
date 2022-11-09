#include "logger/Logger.h"
#include "export.h"
#include <chrono>
#include <sys/stat.h>
#include "pkt/Packet.h"


char host[50] = "";
char filename[1000] = "1.txt";
int port = 69;
char mode = TMOctet;

long gfs() {
    struct stat stat_buf{};
    int rc = stat(filename, &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

std::stringstream log_stream;

long total_block = TF_UNKNOWN_BLOCK_NO;

void callback(callback_data data) {
    if (data.type == TF_START) {
        logger.WriteLog(log_stream << "Start transfer, total block:" << data.value);
        total_block = data.value;
    } else if (data.type == TF_PROGRESS) {
        if (total_block != TF_UNKNOWN_BLOCK_NO) {
            log_stream << "Writing, now progress:" << (double) data.value / (double) total_block;
        } else {
            log_stream << "Reading, now block number:" << data.value;
        }
        logger.WriteLog(log_stream);
    } else if (data.type == TF_END) {
        logger.WriteLog(log_stream << "Transferred, total block:" << data.value);
    } else if (data.type == TF_TRANS_ERR) {
        logger.WriteLog(log_stream << "Transfer error, now block number:" << data.value);
    }
}

int main() {
    using namespace std::chrono;
    long fsize;
    duration<double, std::milli> c{};
    high_resolution_clock::time_point stime;
    high_resolution_clock::time_point etime;
    int type;
    char mode_temp[200];
    while (true) {
        printf("1. init with host name and port\n"
               "2. read from host with filename provided\n"
               "3. write to host with filename provided\n>>: ");
        scanf("%d", &type);
        switch (type) {
            case 1:
                printf("input host and port, split with Space\n>>: ");
                scanf("%s %d", host, &port);
                if (port > 65535 || port < 0) {
                    printf("unexpected dst port");
                    exit(-1);
                }
                if (tf_init(host, port) == -1) perror("init error");
                break;
            case 2:
                printf("input filename and mode, split with Space\n>>: ");
                scanf("%s %s", filename, mode_temp);
                if (strcmp(mode_temp, "ascii") == 0) {
                    mode = TMAscii;
                } else if (strcmp(mode_temp, "octet") == 0) {
                    mode = TMOctet;
                } else {
                    printf("unexpected trans mode");
                    exit(-1);
                }

                stime = high_resolution_clock::now();
                tf_read(filename, mode, callback);
                etime = high_resolution_clock::now();
                //end
                fsize = gfs();
                c = (etime - stime);
                printf("%lf Kb/s\n", ((double) fsize * 1000 / 1024) * 8 / c.count());
                break;
            case 3:
                printf("input filename and mode, split with Space\n>>: ");
                scanf("%s %s", filename, mode_temp);
                if (strcmp(mode_temp, "ascii") == 0) {
                    mode = TMAscii;
                } else if (strcmp(mode_temp, "octet") == 0) {
                    mode = TMOctet;
                } else {
                    printf("unexpected trans mode");
                    exit(-1);
                }
                stime = high_resolution_clock::now();
                tf_write(filename, mode, callback);
                etime = high_resolution_clock::now();
                //end
                fsize = gfs();
                c = (etime - stime);
                printf("%lf Kb/s\n", ((double) fsize * 1000 / 1024) * 8 / c.count());
                break;
            default:
                return 1;
        }
    }
}
