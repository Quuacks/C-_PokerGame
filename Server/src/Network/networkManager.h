#pragma once
#include <string>
#include <functional>
#include <chrono>
#include <thread>
#include <WS2tcpip.h>
#include <iostream>
#include <vector>
#include <game/player.h>

class NetworkManager
{ 
public:
    NetworkManager();
    ~NetworkManager();

    bool Initialize(int port);

    // Called every frame by Application::MainLoop to poll for data/connections
    void Update(std::vector<Player>& authenticatedPlayers,
                std::function<void(SOCKET, const std::string&)> rawCallback,
                std::function<void(Player&, const std::string&)> playerCallback);

    // Utility to send data back to a specific socket
    void SendToClient(SOCKET clientSocket, const std::string& message);

    // Cleanly shuts down all active sockets
    void Shutdown();

    void MoveRawSocketToPlayer(SOCKET socket);

private:
    SOCKET m_ListeningSocket;
    std::vector<SOCKET> m_RawSockets; // Keeps track of all connected poker players

    void HandleNewConnections();
    void PollRawSockets(std::function<void(SOCKET, const std::string&)> rawCallback);
    void PollAuthenticatedPlayers(std::vector<Player>& authenticatedPlayers,
                                 std::function<void(Player&, const std::string&)> playerCallback);
};

