#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include "../GameState.h"


class NetworkClient;

// 3. The Network Bridge Function (The line causing your error)
void SetNetworkClientForUI(NetworkClient* client);
void UpdateUIFromGameState(HWND hWnd, const GameState& state);

std::wstring Utf8ToWide(const std::string& text);
// 4. Core Win32 Setup Functions
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 5. UI Mutator & Rendering Functions
void UpdateUIFromGameState(HWND hWnd, const GameState& state);

void AddStatusMessage(const std::wstring& message);
void AddStatusMessageFromUtf8(const std::string& message);

void StartGdiPlus();
void ShutdownGdiPlus();
void LoadChipAssets();

void SetHeroTurn(bool isHeroTurn);

void ShowRaiseControls(bool show);
void UpdateRaiseAmountText();
int GetRaiseAmountFromEdit();
void PositionControls(HWND hWnd);
void FillHandsCombo();
