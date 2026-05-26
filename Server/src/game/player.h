#pragma once
#include <string>
#include <vector>
#include <WS2tcpip.h>
#include "GameStructures.h"

class Player {
public:
    Player(SOCKET socket, std::string username)
        : m_Username(std::move(username)), m_Socket(socket), m_Chips(2000){ }

    ~Player() = default;

    std::string GetUsername() const {
        return m_Username;
    }

    SOCKET getSocket() const {
        return m_Socket;
    }

    int getChips() {
        return m_Chips;
    }
    void AddChips(int amount) {
        m_Chips += amount;
    }
    void RemoveChips(int amount) {
        m_Chips -= amount;
    }

    bool IsFolded() const {
        return m_IsFolded;
    }
    void SetFolded(bool folded) {
        m_IsFolded = folded;
    }

    void DealCard(const ServerCard& card) {
        m_HoleCards.push_back(card);
    }
    void ClearHand() {
        m_HoleCards.clear(); m_IsFolded = false;
    }
    const std::vector<ServerCard>& GetHoleCards() const {
        return m_HoleCards;
    }

    void AppendToNetworkBuffer(const std::string& data) {
        m_NetworkBuffer += data;
    }
    std::string& GetNetworkBuffer() {
        return m_NetworkBuffer;
    }
    void ClearNetworkBuffer() {
        m_NetworkBuffer.clear();
    }

private:
    SOCKET m_Socket;
    int m_Chips = 0;
    std::string m_Username;
    bool m_IsFolded = false;

    std::vector<ServerCard> m_HoleCards;
    std::string m_NetworkBuffer;
};
