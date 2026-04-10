#pragma once
#include <string>
#include <functional>
#include <chrono>
#include <thread>

class NetworkManager
{ 
public:
    explicit NetworkManager() 
    {
        // Initialize server
    }

    void SendMessage(std::string& message) 
    {

    }

    void Update(std::function<void(const std::string&)> onReceiveMessage) 
    {
        AcceptIncomingConnections();
        std::string msgBuffer;
        bool received = TryAcceptMessages(msgBuffer);
        if (received) {
            onReceiveMessage(msgBuffer);
        }
    }

private:
    void AcceptIncomingConnections() 
    {

    }

    // returns true if a message was received, else - false.
    bool TryAcceptMessages(std::string& messageStr) 
    {

    }
};

