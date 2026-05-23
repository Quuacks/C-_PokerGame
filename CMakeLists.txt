cmake_minimum_required(VERSION 3.15)
project(PokerGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the server executable and tell CMake where all the source files live
add_executable(PokerServer 
    Server/main.cpp 
    Server/application.cpp
    Server/Network/NetworkManager.cpp
    # Add your other future files here as you write them:
    # Server/Game/player.cpp
    # Server/Game/room.cpp
)

# Tell CMake where to look for your header files so #include "application.h" works
target_include_directories(PokerServer PRIVATE Server)

# Crucial Windows step: Link the Winsock2 library so network functions work
if(WIN32)
    target_link_libraries(PokerServer PRIVATE ws2_32)
endif()
