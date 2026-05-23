#pragma once
#include <string>
#include <functional>
#include <chrono>
#include <thread>
#include <WS2tcpip.h>
#include <iostream>

class NetworkManager
{ 
public:
    NetworkManager();
    ~NetworkManager();

    bool Initialize(int port);

    // Called every frame by Application::MainLoop to poll for data/connections
    void Update(std::function<void(const std::string&)> messageCallback);

    // Utility to send data back to a specific socket
    void SendToClient(SOCKET clientSocket, const std::string& message);

    // Cleanly shuts down all active sockets
    void Shutdown();

private:
    SOCKET m_ListeningSocket;
    std::vector<SOCKET> m_ClientSockets; // Keeps track of all connected poker players

    void HandleNewConnections();
    void PollClientData(std::function<void(const std::string&)> messageCallback);
};

