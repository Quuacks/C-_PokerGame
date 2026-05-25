#include "../framework.h"
#include "UI.h"
#include "../Network/NetworkClient.h" // Points to your client wrapper
#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdlib>
#include <nlohmann/json.hpp>

#pragma comment(lib, "comctl32.lib")

using json = nlohmann::json;

#define ID_BTN_RAISE          1001
#define ID_BTN_CALL           1002
#define ID_BTN_CHECK          1003
#define ID_BTN_FOLD           1004

#define ID_SLIDER             2001
#define ID_EDIT_RAISE_AMOUNT  2002
#define ID_BTN_CONFIRM_RAISE  2003
#define ID_BTN_CANCEL_RAISE   2004

#define ID_COMBO_HAND         3001
#define ID_LIST_PLAYERS       3002
#define ID_STATUS_LOG         3003

// Global handles required for Win32 callbacks
HINSTANCE hInst;
HWND g_hSlider;
HWND g_editRaiseAmount;
HWND g_btnConfirmRaise;
HWND g_btnCancelRaise;
HWND g_btnRaise, g_btnCall, g_btnCheck, g_btnFold;
HWND g_comboHands, g_listPlayers;
HWND g_statusLog;

const int MIN_RAISE = 0;
const int MAX_RAISE = 10000; // Increased to match typical chip stacks
int g_potAmount = 0;
std::vector<Card> g_boardCards;
std::vector<Card> g_heroCards;
std::vector<std::wstring> g_players;
int g_selectedComboIndex = 0;

// SINGLE POINT OF CONTROL: Global pointer linking back to network engine
NetworkClient* g_NetworkClient = nullptr;

std::unordered_map<int, HBITMAP> g_cardBitmaps;
HBITMAP g_backsideBitmap = nullptr;

int g_suitBase[4] = {
    1,   // Hearts
    15,  // Spades
    29,  // Diamonds
    43   // Clubs
};

const int CARD_BACK_INDEX = 28;


void SetNetworkClientForUI(NetworkClient* client) {
    g_NetworkClient = client;
}

std::wstring Utf8ToWide(const std::string& text)
{
    if (text.empty())
        return L"";

    int sizeNeeded = MultiByteToWideChar(
        CP_UTF8,
        0,
        text.c_str(),
        (int)text.size(),
        nullptr,
        0
    );

    if (sizeNeeded <= 0)
        return std::wstring(text.begin(), text.end());

    std::wstring result(sizeNeeded, L'\0');

    MultiByteToWideChar(
        CP_UTF8,
        0,
        text.c_str(),
        (int)text.size(),
        &result[0],
        sizeNeeded
    );

    return result;
}

void AddStatusMessage(const std::wstring& message)
{
    if (!g_statusLog)
        return;

    SendMessage(g_statusLog, LB_ADDSTRING, 0, (LPARAM)message.c_str());

    int count = (int)SendMessage(g_statusLog, LB_GETCOUNT, 0, 0);

    if (count > 100)
    {
        SendMessage(g_statusLog, LB_DELETESTRING, 0, 0);
        count--;
    }

    SendMessage(g_statusLog, LB_SETTOPINDEX, count - 1, 0);
}

void LoadCardAssets()
{
    // Load backside
    {
        wchar_t filename[64];
        wsprintf(filename, L"assets/cards/%02d_kerenel_Cards.png", CARD_BACK_INDEX);

        g_backsideBitmap = (HBITMAP)LoadImageW(
            nullptr, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE
        );
    }

    // Load all suit cards
    for (int suit = 0; suit < 4; suit++)
    {
        int base = g_suitBase[suit];

        for (int rank = 1; rank <= 13; rank++)
        {
            int fileIndex = base + (rank - 1);

            wchar_t filename[64];
            wsprintf(filename, L"assets/cards/%02d_kerenel_Cards.png", fileIndex);

            HBITMAP bmp = (HBITMAP)LoadImageW(
                nullptr, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE
            );

            if (bmp)
                g_cardBitmaps[fileIndex] = bmp;
        }
    }

    AddStatusMessage(L"Card assets loaded.");
}

int GetCardFileIndex(const Card& c)
{
    if (c.rank == 0)
        return CARD_BACK_INDEX;

    return g_suitBase[c.suit] + (c.rank - 1);
}

void DrawCard(HDC hdc, int x, int y, int w, int h, const Card& c)
{
    int index = GetCardFileIndex(c);

    HBITMAP bmp = (index == CARD_BACK_INDEX)
        ? g_backsideBitmap
        : g_cardBitmaps[index];

    if (!bmp)
    {
        Rectangle(hdc, x, y, x + w, y + h);
        return;
    }

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, bmp);

    BITMAP bm;
    GetObject(bmp, sizeof(bm), &bm);

    StretchBlt(
        hdc, x, y, w, h,
        memDC, 0, 0, bm.bmWidth, bm.bmHeight,
        SRCCOPY
    );

    SelectObject(memDC, oldBmp);
    DeleteDC(memDC);
}

void AddStatusMessageFromUtf8(const std::string& message)
{
    AddStatusMessage(Utf8ToWide(message));
}
void ShowRaiseControls(bool show) {
    int command = show ? SW_SHOW : SW_HIDE;
    ShowWindow(g_hSlider, command);
    ShowWindow(g_editRaiseAmount, command);
    ShowWindow(g_btnConfirmRaise, command);
    ShowWindow(g_btnCancelRaise, command);
}

void UpdateRaiseAmountText() {
    int amount = (int)SendMessage(g_hSlider, TBM_GETPOS, 0, 0);
    wchar_t buffer[32];
    wsprintf(buffer, L"%d", amount);
    SetWindowText(g_editRaiseAmount, buffer);
}

int GetRaiseAmountFromEdit() {
    wchar_t buffer[32];
    GetWindowText(g_editRaiseAmount, buffer, 32);
    int amount = _wtoi(buffer);
    if (amount < MIN_RAISE) amount = MIN_RAISE;
    if (amount > MAX_RAISE) amount = MAX_RAISE;
    return amount;
}

void PositionControls(HWND hWnd) {
    RECT rc;
    GetClientRect(hWnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    int bottomHeight = 80;
    int sideWidth = w / 6;
    int btnWidth = w / 10;
    int btnHeight = 40;
    int margin = 20;

    int yButtons = h - bottomHeight + (bottomHeight - btnHeight) / 2;

    MoveWindow(g_btnRaise, margin, yButtons, btnWidth, btnHeight, TRUE);
    MoveWindow(g_btnCall, margin + btnWidth + 10, yButtons, btnWidth, btnHeight, TRUE);
    MoveWindow(g_btnCheck, margin + 2 * (btnWidth + 10), yButtons, btnWidth, btnHeight, TRUE);
    MoveWindow(g_btnFold, margin + 3 * (btnWidth + 10), yButtons, btnWidth, btnHeight, TRUE);

    int raiseY = yButtons - 45;
    MoveWindow(g_hSlider, margin, raiseY, btnWidth * 2, 30, TRUE);
    MoveWindow(g_editRaiseAmount, margin + btnWidth * 2 + 10, raiseY, 70, 25, TRUE);
    MoveWindow(g_btnConfirmRaise, margin + btnWidth * 2 + 90, raiseY, 60, 25, TRUE);
    MoveWindow(g_btnCancelRaise, margin + btnWidth * 2 + 155, raiseY, 70, 25, TRUE);

    MoveWindow(g_comboHands, margin, margin, sideWidth - 2 * margin, 200, TRUE);

    int statusTop = margin + 220;
    int statusHeight = h - bottomHeight - statusTop - margin;

    if (statusHeight < 80)
        statusHeight = 80;

    MoveWindow(
        g_statusLog,
        margin,
        statusTop,
        sideWidth - 2 * margin,
        statusHeight,
        TRUE
    );

    MoveWindow(
        g_listPlayers,
        w - sideWidth + margin,
        margin,
        sideWidth - 2 * margin,
        h - 2 * margin - bottomHeight,
        TRUE
    );
}

void FillHandsCombo() {
    const wchar_t* hands[] = {
        L"High Card", L"One Pair", L"Two Pair", L"Three of a Kind",
        L"Straight", L"Flush", L"Full House", L"Four of a Kind",
        L"Straight Flush", L"Royal Flush"
    };
    for (auto& h : hands)
        SendMessage(g_comboHands, CB_ADDSTRING, 0, (LPARAM)h);
    SendMessage(g_comboHands, CB_SETCURSEL, g_selectedComboIndex, 0);
}

// UI State Mutator called explicitly whenever client receives a server update packet
void UpdateUIFromGameState(HWND hWnd, const GameState& state) {
    g_boardCards = state.boardCards;
    g_heroCards = state.heroCards;
    g_potAmount = state.pot;
    g_players = state.players;
    g_selectedComboIndex = state.selectedComboIndex;

    SendMessage(g_comboHands, CB_SETCURSEL, g_selectedComboIndex, 0);
    SendMessage(g_listPlayers, LB_RESETCONTENT, 0, 0);
    for (auto& p : g_players)
        SendMessage(g_listPlayers, LB_ADDSTRING, 0, (LPARAM)p.c_str());

    InvalidateRect(hWnd, nullptr, TRUE);
}

void DrawTable(HDC hdc, RECT rc) {
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    int bottomHeight = 80;
    int sideWidth = w / 6;

    RECT center;
    center.left = rc.left + sideWidth;
    center.right = rc.right - sideWidth;
    center.top = rc.top + 60;
    center.bottom = rc.bottom - bottomHeight - 20;

    HBRUSH tableBrush = CreateSolidBrush(RGB(0, 100, 0));
    FillRect(hdc, &center, tableBrush);
    DeleteObject(tableBrush);

    wchar_t potBuf[64];
    wsprintf(potBuf, L"Pot: %d", g_potAmount);
    SetBkMode(hdc, TRANSPARENT);
    RECT potRect = { (w / 2) - 100, rc.top + 10, (w / 2) + 100, rc.top + 40 };
    DrawText(hdc, potBuf, -1, &potRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    int cardW = 60;
    int cardH = 90;
    int spacing = 10;

    int boardCount = (int)g_boardCards.size();
    int totalBoardWidth = boardCount * cardW + (boardCount - 1) * spacing;
    int startX = (center.left + center.right - totalBoardWidth) / 2;
    int y = center.top + 40;

    for (int i = 0; i < boardCount; ++i)
        DrawCard(hdc, startX + i * (cardW + spacing), y, cardW, cardH, g_boardCards[i]);

    int heroCount = (int)g_heroCards.size();
    int totalHeroWidth = heroCount * cardW + (heroCount - 1) * spacing;
    int heroX = (center.left + center.right - totalHeroWidth) / 2;
    int heroY = center.bottom - cardH - 40;

    for (int i = 0; i < heroCount; ++i)
        DrawCard(hdc, heroX + i * (cardW + spacing), heroY, cardW, cardH, g_heroCards[i]);
}

// Win32 Windows Event Callback Interceptor
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SIZE:
        PositionControls(hWnd);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_BTN_RAISE:
            ShowRaiseControls(true);
            UpdateRaiseAmountText();
            AddStatusMessage(L"Raise selected. Choose an amount.");
            break;

        case ID_EDIT_RAISE_AMOUNT:
            if (HIWORD(wParam) == EN_CHANGE) {
                int amount = GetRaiseAmountFromEdit();
                SendMessage(g_hSlider, TBM_SETPOS, TRUE, amount);
            }
            break;

        case ID_BTN_CONFIRM_RAISE: {
            int raiseAmount = GetRaiseAmountFromEdit();
            ShowRaiseControls(false);

            wchar_t statusBuffer[128];
            wsprintf(statusBuffer, L"Raise amount selected: %d", raiseAmount);
            AddStatusMessage(statusBuffer);

            // NETWORK LINK: Fire structural RAISE request
            if (g_NetworkClient) {
                json packet = {
                    {"type", "RAISE"},
                    {"data", {{"amount", raiseAmount}}}
                };
                //g_NetworkClient->SendRequest(packet.dump() + "\n");
            }
            break;
        }

        case ID_BTN_CANCEL_RAISE:
            ShowRaiseControls(false);
            AddStatusMessage(L"Raise cancelled.");
            break;

        case ID_BTN_CALL:
            ShowRaiseControls(false);
            AddStatusMessage(L"Call selected.");

            if (g_NetworkClient) {
                json packet = { {"type", "CALL"}, {"data", json::object()} };
                //g_NetworkClient->SendRequest(packet.dump() + "\n");
            }
            break;

        case ID_BTN_CHECK:
            ShowRaiseControls(false);
            AddStatusMessage(L"Check selected.");

            if (g_NetworkClient) {
                json packet = { {"type", "CHECK"}, {"data", json::object()} };
                //g_NetworkClient->SendRequest(packet.dump() + "\n");
            }
            break;

        case ID_BTN_FOLD:
            ShowRaiseControls(false);
            AddStatusMessage(L"Fold selected.");

            if (g_NetworkClient) {
                json packet = { {"type", "FOLD"}, {"data", json::object()} };
                //g_NetworkClient->SendRequest(packet.dump() + "\n");
            }
            break;
        }
        break;

    case WM_HSCROLL:
        if ((HWND)lParam == g_hSlider) {
            UpdateRaiseAmountText();
        }
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);
        DrawTable(hdc, rc);
        EndPaint(hWnd, &ps);
    }
                 break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = L"PokerUI";
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance;
    HWND hWnd = CreateWindowW(L"PokerUI", L"Poker Game",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 1280, 720,
        nullptr, nullptr, hInstance, nullptr);
    if (!hWnd) return FALSE;

    g_btnRaise = CreateWindow(L"BUTTON", L"Raise", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)ID_BTN_RAISE, hInst, nullptr);
    g_btnCall = CreateWindow(L"BUTTON", L"Call", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)ID_BTN_CALL, hInst, nullptr);
    g_btnCheck = CreateWindow(L"BUTTON", L"Check", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)ID_BTN_CHECK, hInst, nullptr);
    g_btnFold = CreateWindow(L"BUTTON", L"Fold", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)ID_BTN_FOLD, hInst, nullptr);

    g_hSlider = CreateWindowEx(0, TRACKBAR_CLASS, L"", WS_CHILD | TBS_AUTOTICKS, 0, 0, 0, 0, hWnd, (HMENU)ID_SLIDER, hInst, nullptr);
    SendMessage(g_hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(MIN_RAISE, MAX_RAISE));
    SendMessage(g_hSlider, TBM_SETPOS, TRUE, 10);

    g_editRaiseAmount = CreateWindowW(L"EDIT", L"10", WS_CHILD | WS_BORDER | ES_CENTER | ES_NUMBER, 0, 0, 0, 0, hWnd, (HMENU)ID_EDIT_RAISE_AMOUNT, hInst, nullptr);
    g_btnConfirmRaise = CreateWindowW(L"BUTTON", L"OK", WS_CHILD | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)ID_BTN_CONFIRM_RAISE, hInst, nullptr);
    g_btnCancelRaise = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | BS_PUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)ID_BTN_CANCEL_RAISE, hInst, nullptr);

    ShowRaiseControls(false);

    g_comboHands = CreateWindow(
        WC_COMBOBOX,
        L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        0, 0, 0, 0,
        hWnd,
        (HMENU)ID_COMBO_HAND,
        hInst,
        nullptr
    );

    g_statusLog = CreateWindow(
        WC_LISTBOX,
        L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
        0, 0, 0, 0,
        hWnd,
        (HMENU)ID_STATUS_LOG,
        hInst,
        nullptr
    );

    g_listPlayers = CreateWindow(
        WC_LISTBOX,
        L"",
        WS_CHILD | WS_VISIBLE | LBS_NOTIFY,
        0, 0, 0, 0,
        hWnd,
        (HMENU)ID_LIST_PLAYERS,
        hInst,
        nullptr
    );
    FillHandsCombo();
    PositionControls(hWnd);

    AddStatusMessage(L"UI started.");
    AddStatusMessage(L"Waiting for server messages...");

    ShowWindow(hWnd, nCmdShow);
    LoadCardAssets();
    UpdateWindow(hWnd);

    return TRUE;
}
