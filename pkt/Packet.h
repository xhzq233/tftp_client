#ifndef TFTPPACKET_H
#define TFTPPACKET_H

#include <sstream>

/*Macros for TFTP opcodes ,error,maximum size of packet and data. */
enum {
    TMOctet,
    TMAscii
};

bool IsRRQ(const char *packet);

bool IsWRQ(const char *packet);

bool IsData(const char *packet, short block_no);

bool IsACK(const char *packet, short block_no);

bool IsError(const char *packet);

const char *ErrorMessage(const char *packet);

const char *TftpData(const char *packet);


//maximum size of packets
#define TFTP_PACKET_MAX_SIZE 1024
//Maximum Size of file name
#define FILE_NAME_MAX_SIZE 1010


/*A class for framing and deframing of TFTP packets wich uses the interface class "Ipacket".
All packets framed will follow the TFTP protocol.*/

class Packet {
public:
    char *data;            //Data in the data packet.
    size_t len = 0;

    explicit Packet(char *data) : data(data) {}

    /*For creating request to write into a file .

      2 bytes     string    1 byte     string   1 byte
      ------------------------------------------------
      | Opcode |  Filename  |   0  |    Mode    |   0  |
      ------------------------------------------------	*/
    static Packet &CreateWrq(const char *filename, char mode);

    /*For creating  request to read a file .
      2 bytes     string    1 byte     string   1 byte
      ------------------------------------------------
      | Opcode |  Filename  |   0  |    Mode    |   0  |
      ------------------------------------------------	*/

    static Packet &CreateRrq(const char *filename, char mode);

    /*For creating a data packet that will contain 512 bytes of data.

      2 bytes     2 bytes      n bytes
      ----------------------------------
      | Opcode |   Block #  |   Data
      ----------------------------------	*/
    static Packet &CreateData(short block_no, const char *payload, size_t len);

    /*For creating the aknowledgement responce packet

     2 bytes     2 bytes
     ---------------------
     | Opcode |   Block #  |
     ---------------------*/
    static Packet &CreateAck(short block_no);

    /*For creating a error packet.

    2 bytes     2 bytes      string    1 byte
    -----------------------------------------
    | Opcode |  ErrorCode |   ErrMsg   |   0  |
    -----------------------------------------		*/
    static Packet &CreateError(short code, const char *msg);
};


#endif
