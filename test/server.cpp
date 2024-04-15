#include "TcpServer.h"

int main()
{
    TcpServer server(PORT);

    server.Run();

    printf("server stop runing...\n");
    return 0;
}