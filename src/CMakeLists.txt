add_library(unpWork
        TcpClient.cpp
        TcpServer.cpp)

target_link_libraries(unpWork PRIVATE thread_poll locker utils pthread)