cmake_minimum_required(VERSION 3.22)
project(tftp_client VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)

# executable setting
add_executable(
        tftp_client
        logger/Logger.cpp
        logger/Logger.h
        export.cpp
        export.h
        pkt/Packet.h
        pkt/Packet.cpp
        main.cpp
)

install(TARGETS tftp_client)

# lib setting
#add_library(
#        tftp_client SHARED
#        logger/Logger.cpp
#        logger/Logger.h
#        export.cpp
#        export.h
#        pkt/Packet.h
#        pkt/Packet.cpp
#)
#set_target_properties(
#        tftp_client PROPERTIES
#        PUBLIC_HEADER export.h
#        OUTPUT_NAME "tf"
#)