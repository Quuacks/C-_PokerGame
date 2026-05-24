#include <iostream>
#include "Network/NetworkClient.h"
#include <WS2tcpip.h>
#include <thread>
#include <chrono>
#include <string>

#pragma comment(lib, "ws2_32.lib")

int main() {
    WSADATA data;
    WORD ver = MAKEWORD(2, 2);

    if (WSAStartup(ver, &data) != 0) {
        std::cerr << "[MAIN] Can't start Winsock \n";
    }

    {
        NetworkClient client;

        if (!client.ConnectToServer("127.0.0.1", 54000)) {
            std::cerr << "Couldnt connect to server\n";
            return 1;
        }

        std::cout << "CONECTED\n";

        std::string userInput;

        while (true) {
            client.Update([](const std::string& serverMsg) {
                std::cout << "\n[From Server]: " << serverMsg << "\n";
                std::cout.flush();
                });

            std::cout << "> ";
            std::getline(std::cin, userInput);

            if (!userInput.empty()) {
                client.SendRequest(userInput);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        client.Disconnect();
    }

    WSACleanup();
    return 0;
}
