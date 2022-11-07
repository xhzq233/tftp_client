#define FILE_NAME_MAX_SIZE 1010

#include "logger/Logger.h"
#include "export.h"
#include <chrono>
#include <sys/stat.h>
#include <execinfo.h>
#include "pkt/Packet.h"

struct command_line_data {
    char host[50] = "";
    char filename[FILE_NAME_MAX_SIZE] = "1.txt";
    int port = 69;
    char op = TMRead;
    char mode = TMOctet;
} arguments;

void error() {
    int size = 16;
    void *array[16];
    int stack_num = backtrace(array, size);
    auto trace = backtrace_symbols(array, stack_num);
    for (int i = 0; i < stack_num; ++i) {
        puts(trace[i]);
        putchar('\n');
    }
    exit(-1);
}

void parse(int argc, char **argv) {
    std::stringstream ss;
    ss << "arguments count: " << argc << " ";
    for (int i = 0; i < argc; ++i) {
        ss << argv[i] << ' ';
    }
    logger.WriteLog(ss);

    if (argc <= 2) error();
    //host & filename 必选
    strcpy(arguments.host, argv[1]);
    strcpy(arguments.filename, argv[2]);

    if (argc > 3) {
        arguments.port = atoi(argv[3]);
        if (arguments.port > 65535 || arguments.port < 0) error();
    }

    if (argc > 4) {
        arguments.op = strcmp(argv[4], "write") == 0 ? TMWrite : TMRead;
    }

    if (argc > 5) {
        arguments.mode = strcmp(argv[5], "ascii") == 0 ? TMAscii : TMOctet;
    }
}

long gfs(const char *filename) {
    struct stat stat_buf{};
    int rc = stat(filename, &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

std::stringstream log_stream;

void callback(callback_data data) {
    if (data.type == TF_START) {
        logger.WriteLog(log_stream << "Start transfer, total block:" << data.value);
    } else if (data.type == TF_PROGRESS) {
        logger.WriteLog(log_stream << "Transferring, now block number:" << data.value);
    } else if (data.type == TF_END) {
        logger.WriteLog(log_stream << "Transferred, total block:" << data.value);
    } else if (data.type == TF_TRANS_ERR) {
        logger.WriteLog(log_stream << "Transfer error, now block number:" << data.value);
    }
}

int main(int argc, char **argv) {

    parse(argc, argv);

    /*Initiate the server and clients*/
    if (tf_init(arguments.host, arguments.port) == -1) error();

    if (arguments.op == TMRead) {
        using namespace std::chrono;
        high_resolution_clock::time_point stime = high_resolution_clock::now();
        tf_read(arguments.filename, arguments.mode, callback);
        high_resolution_clock::time_point etime = high_resolution_clock::now();

        //end
        auto fsize = gfs(arguments.filename);
        duration<double, std::milli> c = (etime - stime);
        printf("%lf Kb/s\n", ((double) fsize * 1000 / 1024) * 8 / c.count());

    } else if (arguments.op == TMWrite) {
        using namespace std::chrono;
        high_resolution_clock::time_point stime = high_resolution_clock::now();
        tf_write(arguments.filename, arguments.mode, callback);
        high_resolution_clock::time_point etime = high_resolution_clock::now();

        //end
        auto fsize = gfs(arguments.filename);
        duration<double, std::milli> c = (etime - stime);
        printf("%lf Kb/s\n", ((double) fsize * 1000 / 1024) * 8 / c.count());
    } else error();
}
