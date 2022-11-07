#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>


/*Macro definition for logging*/
#define ASSERT(fun)\
if(!(fun)){        \
    logger.WriteError(msg<<":"<<__FILE__<<":"<<__FUNCTION__<<":"<<__LINE__<<":"); \
    perror("");    \
    assert(fun);   \
}

/*Types of message to be written in log*/
#define INFO   "DEBUG "
#define ERR    "ERROR "

/*singleton class for logging into the file */
class Logger {
public:
    virtual ~Logger();                        //distructor
    static Logger &Instance();                //method to access the private object of the Logger(single instance)
    void WriteLog(const std::string &ss);        //for writing log info into the file using synchronisation
    void WriteError(const std::string &ss);        //for writing errors info

    void WriteError(const std::ostream &ss);        //for writing errors info
    void WriteLog(const std::ostream &ss);

    static int GetSeqNo();                            //for obtaining the sequence of the log information

private:
    static char *GetTimeStamp();    //to get the timestamp of current execution
    std::ofstream m_Stream;            //File stream will be used to write the log information
    Logger();                                //private constructor


    template<class T>
    friend Logger &operator<<(Logger &os, const T &s) {
        os.log(INFO, s);
        return os;
    }

    template<class T>
    void log(const char *label, const T &s) {
        m_Stream << GetSeqNo() << ". [" << label << GetTimeStamp() << "]: " << s << std::endl;
    }
};

static Logger &logger = Logger::Instance();
#endif