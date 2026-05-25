#pragma once

#include <windows.h>
#include <vector>
#include <string>

// 2. Core UI Data Structures
struct Card
{
    int rank;
    int suit;
};

struct GameState
{
    std::vector<Card> boardCards;
    std::vector<Card> heroCards;
    int pot;
    std::vector<std::wstring> players;
    int selectedComboIndex;
};

class NetworkClient;

// 3. The Network Bridge Function (The line causing your error)
void SetNetworkClientForUI(NetworkClient* client);

// 4. Core Win32 Setup Functions
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 5. UI Mutator & Rendering Functions
void UpdateUIFromGameState(HWND hWnd, const GameState& state);

void AddStatusMessage(const std::wstring& message);
void AddStatusMessageFromUtf8(const std::string& message);

void ShowRaiseControls(bool show);
void UpdateRaiseAmountText();
int GetRaiseAmountFromEdit();
void PositionControls(HWND hWnd);
void FillHandsCombo();
