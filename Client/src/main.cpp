#include <iostream>
#include "framework.h"
#include "Network/NetworkClient.h"
#include "UI/UI.h"
#include <WS2tcpip.h>
#include <thread>
#include <chrono>
#include <string>
#include <commctrl.h>
#include <nlohmann/json.hpp>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comctl32.lib")

//Temporary for console along side UI window
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:wWinMainCRTStartup")

using json = nlohmann::json;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    // 1. Allocate a standard console window for tracking debug outputs
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    std::cout << "[CLIENT] Debug engine initialized successfully.\n";

    // 2. Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "[ERROR] Winsock initialization failed.\n";
        return 0;
    }

    // 3. Initialize visual styles for sliders/buttons
    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_BAR_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icex);

    // 4. Initialize Network Client instance
    NetworkClient client;
    SetNetworkClientForUI(&client); // Pass the client pointer to UI backend!

    if (!client.ConnectToServer("127.0.0.1", 54000)) {
        MessageBox(nullptr, L"Failed to connect to poker server!", L"Error", MB_ICONERROR);
        WSACleanup();
        return 0;
    }

    // 5. Send initial JSON login request packet
    json loginPacket = { {"type", "LOGIN"}, {"data", {{"username", "Quuacks"}}} };
    client.SendRequest(loginPacket.dump() + "\n");

    // 6. Register and create the game UI window frame
    MyRegisterClass(hInstance);
    if (!InitInstance(hInstance, nCmdShow)) {
        std::cout << "[ERROR] Window frame allocation failed.\n";
        WSACleanup();
        return FALSE;
    }

    // Capture the primary window handle for refreshing graphics
    HWND mainHWnd = FindWindowW(L"PokerUI", L"Poker Game");

    AddStatusMessage(L"Connected to server.");
    AddStatusMessage(L"Login request sent.");
    AddStatusMessage(L"Client is ready.");

    std::cout << "[CLIENT] Entering unified core engine loop...\n";

    // 7. Core Non-Blocking Unified Message & Network Loop
    MSG msg{};
    while (msg.message != WM_QUIT) {
        // Handle window dragging, clicks, and UI resize actions instantly
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Handle incoming asynchronous network data updates from the server
        client.Update([mainHWnd](const std::string& rawServerMessage) {
            try {
                // You can reuse your json parsing logic right here to process table states!
                std::cout << "[NET] Received payload: " << rawServerMessage << "\n";

                AddStatusMessageFromUtf8("[Server] " + rawServerMessage);
            }
            catch (...) {
                AddStatusMessage(L"Failed to process server message.");
            }
            });
        Sleep(10); // Throttle loop to manage CPU consumption
    }

    // 8. Exit Cleanup
    SetNetworkClientForUI(nullptr);
    client.Disconnect();
    if (fp) fclose(fp);
    FreeConsole();
    WSACleanup();

    return (int)msg.wParam;
}
