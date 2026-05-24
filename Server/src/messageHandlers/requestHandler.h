#pragma once
#include <string>
class RequestHandler
{ 
public:
    virtual void Execute(std::string message) = 0;


};

