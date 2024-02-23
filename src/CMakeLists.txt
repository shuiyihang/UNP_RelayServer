add_library(unpWork
        TcpBase.cpp
        TcpClient.cpp
        TcpServer.cpp)

target_link_libraries(unpWork PRIVATE thread_poll locker pthread)