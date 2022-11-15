#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
#include <iostream>
#include <sstream>

/*Types of message to be written in log*/
#define INFO   "DEBUG "
#define ERR    "ERROR "

/*singleton class for logging into the file */
class Logger {
public:
    virtual ~Logger();                        //distructor
    static Logger &Instance();                //method to access the private object of the Logger(single instance)
    void WriteError(const std::string &ss);        //for writing errors info
    void WriteError(std::ostringstream &ss);        //for writing errors info
    void WriteLog(std::ostringstream &ss);

    static int GetSeqNo();                            //for obtaining the sequence of the log information

private:
    static char *GetTimeStamp();    //to get the timestamp of current execution
    std::ofstream stream;            //File stream will be used to write the log information
    Logger();                                //private constructor

    void log(const char *label, const std::string &s) {
        stream << GetSeqNo() << ". [" << label << GetTimeStamp() << "]: " << s << std::endl;
        printf("%s\n", s.c_str());
    }
};

static Logger &logger = Logger::Instance();
#endif
