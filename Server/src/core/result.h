#pragma once
#include <string>

class ResultError
{
public:
    ResultError(std::string message)
    : m_Message(message) { }

    inline std::string GetMessage() const {
        return m_Message;
    };
private:
    std::string m_Message;
};
