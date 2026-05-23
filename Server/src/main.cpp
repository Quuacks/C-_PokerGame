#include <iostream>
#include <WS2tcpip.h>
#include "Application.h"

#pragma comment(lib, "ws2_32.lib")

int main()
{
    //Initializing winsock
    WSADATA wsData;
    WORD ver = MAKEWORD(2, 2);

    int wsOk = WSAStartup(ver, &wsData);

    if (wsOk != 0) {
        std::cout << "cant initialize winsock" << std::endl;
        return 0;
    }


    {
        Application app;
        app.Start();
    }

    WSACleanup();

    return 0;
}
