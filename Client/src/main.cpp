#include <iostream>
#include "framework.h"
#include "Network/NetworkClient.h"
#include "UI/UI.h"
#include <WS2tcpip.h>
#include <thread>
#include <chrono>
#include <string>
#include <commctrl.h>
#include <json.hpp>
#include "GameState.h"
#include "GameStateParser.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comctl32.lib")

//Temporary for console along side UI window
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:wWinMainCRTStartup")

using json = nlohmann::json;

#define ID_LOGIN_EDIT      6001
#define ID_LOGIN_BUTTON    6002
#define ID_CANCEL_BUTTON   6003

struct LoginPromptData
{
    std::wstring username;
    bool done = false;
    bool cancelled = false;
    HWND editBox = nullptr;
};

std::string WideToUtf8(const std::wstring& text);
std::wstring TrimUsername(const std::wstring& text);
LRESULT CALLBACK LoginWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
std::string ShowLoginPage(HINSTANCE hInstance);

bool TryReadBoolTurnField(const json& data, const char* key, bool& result)
{
    if (data.contains(key) && data[key].is_boolean())
    {
        result = data[key].get<bool>();
        return true;
    }

    return false;
}

bool IsLocalPlayersTurn(const json& data, const GameState& state, const std::string& localUsername)
{
    bool turnValue = false;

    if (TryReadBoolTurnField(data, "isHeroTurn", turnValue))
        return turnValue;

    if (TryReadBoolTurnField(data, "isYourTurn", turnValue))
        return turnValue;

    if (TryReadBoolTurnField(data, "yourTurn", turnValue))
        return turnValue;

    const char* possibleNameFields[] = {
        "currentPlayer",
        "currentPlayerName",
        "currentTurn",
        "turnPlayer"
    };

    for (const char* field : possibleNameFields)
    {
        if (data.contains(field) && data[field].is_string())
        {
            return data[field].get<std::string>() == localUsername;
        }
    }

    const char* possibleIndexFields[] = {
        "currentPlayerIndex",
        "turnPlayerIndex",
        "activePlayerIndex"
    };

    for (const char* field : possibleIndexFields)
    {
        if (data.contains(field) && data[field].is_number_integer())
        {
            int index = data[field].get<int>();

            if (index >= 0 && index < (int)state.players.size())
            {
                return state.players[index] == Utf8ToWide(localUsername);
            }
        }
    }

    return state.isHeroTurn;
}

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

    NetworkClient client;
    SetNetworkClientForUI(&client);

    if (!client.ConnectToServer("127.0.0.1", 54000)) {
        MessageBox(nullptr, L"Failed to connect to poker server!", L"Error", MB_ICONERROR);
        WSACleanup();
        return 0;
    }

    // 4. Initialize Network Client instance
    std::string localUsername = ShowLoginPage(hInstance);

    if (localUsername.empty())
    {
        std::cout << "[CLIENT] Login cancelled.\n";

        if (fp)
            fclose(fp);

        FreeConsole();
        WSACleanup();
        return 0;
    }

    // 5. Send initial JSON login request packet
    json loginPacket = {
        {"type", "LOGIN"},
        {"data", {{"username", localUsername}}}
    };

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
    AddStatusMessageFromUtf8("Logged in as: " + localUsername);
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
        client.Update([mainHWnd, localUsername](const std::string& rawServerMessage) {
            try {
                json packet = json::parse(rawServerMessage);

                if (packet["type"] == "GAME_STATE") {
                    const json& data = packet["data"];

                    GameState state = ParseGameState(data);
                    state.isHeroTurn = IsLocalPlayersTurn(data, state, localUsername);

                    UpdateUIFromGameState(mainHWnd, state);
                    SetHeroTurn(state.isHeroTurn);
                }

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

std::string WideToUtf8(const std::wstring& text)
{
    if (text.empty())
        return "";

    int sizeNeeded = WideCharToMultiByte(
        CP_UTF8,
        0,
        text.c_str(),
        (int)text.size(),
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (sizeNeeded <= 0)
        return "";

    std::string result(sizeNeeded, '\0');

    WideCharToMultiByte(
        CP_UTF8,
        0,
        text.c_str(),
        (int)text.size(),
        &result[0],
        sizeNeeded,
        nullptr,
        nullptr
    );

    return result;
}

std::wstring TrimUsername(const std::wstring& text)
{
    size_t start = text.find_first_not_of(L" \t\r\n");
    size_t end = text.find_last_not_of(L" \t\r\n");

    if (start == std::wstring::npos)
        return L"";

    return text.substr(start, end - start + 1);
}

LRESULT CALLBACK LoginWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LoginPromptData* data = (LoginPromptData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    switch (msg)
    {
    case WM_NCCREATE:
    {
        CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)createStruct->lpCreateParams);
        return TRUE;
    }

    case WM_CREATE:
    {
        data = (LoginPromptData*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

        CreateWindowW(
            L"STATIC",
            L"Enter your username:",
            WS_CHILD | WS_VISIBLE,
            25,
            25,
            300,
            25,
            hWnd,
            nullptr,
            nullptr,
            nullptr
        );

        data->editBox = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            25,
            55,
            300,
            26,
            hWnd,
            (HMENU)ID_LOGIN_EDIT,
            nullptr,
            nullptr
        );

        SendMessage(data->editBox, EM_SETLIMITTEXT, 24, 0);

        CreateWindowW(
            L"BUTTON",
            L"Login",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            95,
            100,
            90,
            32,
            hWnd,
            (HMENU)ID_LOGIN_BUTTON,
            nullptr,
            nullptr
        );

        CreateWindowW(
            L"BUTTON",
            L"Cancel",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            195,
            100,
            90,
            32,
            hWnd,
            (HMENU)ID_CANCEL_BUTTON,
            nullptr,
            nullptr
        );

        SetFocus(data->editBox);
        return 0;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case ID_LOGIN_BUTTON:
        {
            wchar_t buffer[128];
            GetWindowTextW(data->editBox, buffer, 128);

            std::wstring username = TrimUsername(buffer);

            if (username.empty())
            {
                MessageBoxW(
                    hWnd,
                    L"Please enter a username.",
                    L"Username required",
                    MB_OK | MB_ICONWARNING
                );

                SetFocus(data->editBox);
                return 0;
            }

            data->username = username;
            data->done = true;
            DestroyWindow(hWnd);
            return 0;
        }

        case ID_CANCEL_BUTTON:
            data->cancelled = true;
            data->done = true;
            DestroyWindow(hWnd);
            return 0;
        }

        break;
    }

    case WM_CLOSE:
        data->cancelled = true;
        data->done = true;
        DestroyWindow(hWnd);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

std::string ShowLoginPage(HINSTANCE hInstance)
{
    const wchar_t* loginClassName = L"PokerLoginWindow";

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = LoginWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = loginClassName;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassExW(&wc);

    LoginPromptData data;

    HWND loginWindow = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        loginClassName,
        L"Poker Login",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        370,
        180,
        nullptr,
        nullptr,
        hInstance,
        &data
    );

    if (!loginWindow)
        return "";

    ShowWindow(loginWindow, SW_SHOW);
    UpdateWindow(loginWindow);

    MSG msg{};

    while (!data.done && GetMessage(&msg, nullptr, 0, 0))
    {
        if (!IsDialogMessage(loginWindow, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (data.cancelled)
        return "";

    return WideToUtf8(data.username);
}
