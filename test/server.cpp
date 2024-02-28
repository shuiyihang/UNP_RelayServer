#include "TcpServer.h"

int main()
{
    TcpServer server(PORT);

    server.Run();

    return 0;
}