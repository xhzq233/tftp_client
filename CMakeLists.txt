cmake_minimum_required(VERSION 3.22)
project(tftp_client)

set(CMAKE_CXX_STANDARD 20)

include_directories(.)

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