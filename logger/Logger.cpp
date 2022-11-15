#include"Logger.h"

Logger &Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    char file[30];
    time_t t = time(nullptr);
    struct tm *today = localtime(&t);
    strftime(file, 30, "%d-%m-%Y_%H-%M-%S.log", today);
    stream.open(file, std::fstream::app);
}

Logger::~Logger() {
    stream.close();
}

int Logger::GetSeqNo() {
    static int m_seq = 0;                //sequence of the log information
    m_seq += 1;
    return m_seq;
}

void Logger::WriteError(const std::string &ss) {
    log(ERR, ss);
}


char *Logger::GetTimeStamp() {
    static char tString[80];
    time_t t = time(nullptr);
    struct tm *today = localtime(&t);
    strftime(tString, 80, "%d/%m/%Y %H:%M:%S", today);
    return tString;
}

void Logger::WriteError(std::ostringstream &ss) {
    auto s = ss.str();
    log(ERR, s);
    ss.str("");
}

void Logger::WriteLog(std::ostringstream &ss) {
    auto s = ss.str();
    log(INFO, s);
    ss.str("");
}
