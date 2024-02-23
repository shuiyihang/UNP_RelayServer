#include "TcpServer.h"

int main()
{
    TcpServer server(9877);

    server.Run();

    return 0;
}