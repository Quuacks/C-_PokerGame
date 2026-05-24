#include "framework.h"
#include "UI.h"
#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <string>
#include <cstdlib>

#pragma comment(lib, "comctl32.lib")

#define ID_BTN_RAISE  1001
#define ID_BTN_CALL   1002
#define ID_BTN_CHECK  1003
#define ID_BTN_FOLD   1004

#define ID_SLIDER     2001
#define ID_EDIT_RAISE_AMOUNT  2002
#define ID_BTN_CONFIRM_RAISE  2003
#define ID_BTN_CANCEL_RAISE   2004

#define ID_COMBO_HAND 3001
#define ID_LIST_PLAYERS 3002

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

HINSTANCE hInst;

HWND g_hSlider;
HWND g_editRaiseAmount;
HWND g_btnConfirmRaise;
HWND g_btnCancelRaise;

HWND g_btnRaise, g_btnCall, g_btnCheck, g_btnFold;
HWND g_comboHands, g_listPlayers;
const int MIN_RAISE = 0;
const int MAX_RAISE = 100;
int g_potAmount = 0;
std::vector<Card> g_boardCards;
std::vector<Card> g_heroCards;
std::vector<std::wstring> g_players;
int g_selectedComboIndex = 0;

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void ShowRaiseControls(bool show)
{
    int command = show ? SW_SHOW : SW_HIDE;

    ShowWindow(g_hSlider, command);
    ShowWindow(g_editRaiseAmount, command);
    ShowWindow(g_btnConfirmRaise, command);
    ShowWindow(g_btnCancelRaise, command);
}

void UpdateRaiseAmountText()
{
    int amount = (int)SendMessage(g_hSlider, TBM_GETPOS, 0, 0);

    wchar_t buffer[32];
    wsprintf(buffer, L"%d", amount);

    SetWindowText(g_editRaiseAmount, buffer);
}

int GetRaiseAmountFromEdit()
{
    wchar_t buffer[32];
    GetWindowText(g_editRaiseAmount, buffer, 32);

    int amount = _wtoi(buffer);

    if (amount < MIN_RAISE)
        amount = MIN_RAISE;

    if (amount > MAX_RAISE)
        amount = MAX_RAISE;

    return amount;
}

void PositionControls(HWND hWnd)
{
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

    MoveWindow(
        g_editRaiseAmount,
        margin + btnWidth * 2 + 10,
        raiseY,
        70,
        25,
        TRUE
    );

    MoveWindow(
        g_btnConfirmRaise,
        margin + btnWidth * 2 + 90,
        raiseY,
        60,
        25,
        TRUE
    );

    MoveWindow(
        g_btnCancelRaise,
        margin + btnWidth * 2 + 155,
        raiseY,
        70,
        25,
        TRUE
    );
    MoveWindow(g_comboHands, margin, margin, sideWidth - 2 * margin, 200, TRUE);
    MoveWindow(g_listPlayers, w - sideWidth + margin, margin, sideWidth - 2 * margin, h - 2 * margin - bottomHeight, TRUE);
}

void FillHandsCombo()
{
    const wchar_t* hands[] = {
        L"High Card",
        L"One Pair",
        L"Two Pair",
        L"Three of a Kind",
        L"Straight",
        L"Flush",
        L"Full House",
        L"Four of a Kind",
        L"Straight Flush",
        L"Royal Flush"
    };
    for (auto& h : hands)
        SendMessage(g_comboHands, CB_ADDSTRING, 0, (LPARAM)h);
    SendMessage(g_comboHands, CB_SETCURSEL, g_selectedComboIndex, 0);
}

void UpdateUIFromGameState(HWND hWnd, const GameState& state)
{
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

void DrawCard(HDC hdc, int x, int y, int w, int h, const Card& c)
{
    Rectangle(hdc, x, y, x + w, y + h);

    wchar_t buf[16];
    const wchar_t* ranks = L" A23456789TJQK";
    const wchar_t* suits[] = { L"♣", L"♦", L"♥", L"♠" };

    wchar_t r = ranks[c.rank];
    const wchar_t* s = suits[c.suit % 4];
    wsprintf(buf, L"%c%s", r, s);

    SetBkMode(hdc, TRANSPARENT);
    RECT cardRect = { x + 5, y + 5, x + w - 5, y + h - 5 };
    DrawText(hdc, buf, -1, &cardRect, DT_LEFT | DT_TOP);
}

void DrawTable(HDC hdc, RECT rc)
{
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

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_BAR_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icex);

    MyRegisterClass(hInstance);
    if (!InitInstance(hInstance, nCmdShow)) return FALSE;

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = L"PokerUI";
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(L"PokerUI", L"Poker Game",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 1280, 720,
        nullptr, nullptr, hInstance, nullptr);
    if (!hWnd) return FALSE;

    g_btnRaise = CreateWindow(L"BUTTON", L"Raise",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 0, 0, hWnd, (HMENU)ID_BTN_RAISE, hInst, nullptr);
    g_btnCall = CreateWindow(L"BUTTON", L"Call",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 0, 0, hWnd, (HMENU)ID_BTN_CALL, hInst, nullptr);
    g_btnCheck = CreateWindow(L"BUTTON", L"Check",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 0, 0, hWnd, (HMENU)ID_BTN_CHECK, hInst, nullptr);
    g_btnFold = CreateWindow(L"BUTTON", L"Fold",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 0, 0, hWnd, (HMENU)ID_BTN_FOLD, hInst, nullptr);

    g_hSlider = CreateWindowEx(0, TRACKBAR_CLASS, L"",
        WS_CHILD | TBS_AUTOTICKS,
        0, 0, 0, 0, hWnd, (HMENU)ID_SLIDER, hInst, nullptr);

    SendMessage(g_hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(MIN_RAISE, MAX_RAISE));
    SendMessage(g_hSlider, TBM_SETPOS, TRUE, 10);

    g_editRaiseAmount = CreateWindowW(
        L"EDIT",
        L"10",
        WS_CHILD | WS_BORDER | ES_CENTER | ES_NUMBER,
        0, 0, 0, 0,
        hWnd,
        (HMENU)ID_EDIT_RAISE_AMOUNT,
        hInst,
        nullptr
    );

    g_btnConfirmRaise = CreateWindowW(
        L"BUTTON",
        L"OK",
        WS_CHILD | BS_PUSHBUTTON,
        0, 0, 0, 0,
        hWnd,
        (HMENU)ID_BTN_CONFIRM_RAISE,
        hInst,
        nullptr
    );

    g_btnCancelRaise = CreateWindowW(
        L"BUTTON",
        L"Cancel",
        WS_CHILD | BS_PUSHBUTTON,
        0, 0, 0, 0,
        hWnd,
        (HMENU)ID_BTN_CANCEL_RAISE,
        hInst,
        nullptr
    );

    ShowRaiseControls(false);

    g_comboHands = CreateWindow(WC_COMBOBOX, L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
        0, 0, 0, 0, hWnd, (HMENU)ID_COMBO_HAND, hInst, nullptr);

    g_listPlayers = CreateWindow(WC_LISTBOX, L"",
        WS_CHILD | WS_VISIBLE | LBS_NOTIFY,
        0, 0, 0, 0, hWnd, (HMENU)ID_LIST_PLAYERS, hInst, nullptr);

    FillHandsCombo();
    PositionControls(hWnd);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    GameState demo;
    demo.pot = 120;
    demo.boardCards = { {10,3},{11,3},{12,3},{13,3},{1,3} };
    demo.heroCards = { {14,3},{14,1} };
    demo.players = { L"Player 1", L"Player 2", L"Player 3" };
    demo.selectedComboIndex = 9;
    UpdateUIFromGameState(hWnd, demo);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
        PositionControls(hWnd);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_BTN_RAISE:
            ShowRaiseControls(true);
            UpdateRaiseAmountText();
            break;

        case ID_EDIT_RAISE_AMOUNT:
        {
            if (HIWORD(wParam) == EN_CHANGE)
            {
                int amount = GetRaiseAmountFromEdit();
                SendMessage(g_hSlider, TBM_SETPOS, TRUE, amount);
            }
            break;
        }

        case ID_BTN_CONFIRM_RAISE:
        {
            int raiseAmount = GetRaiseAmountFromEdit();

            wchar_t buffer[32];
            wsprintf(buffer, L"%d", raiseAmount);

            SetWindowText(g_editRaiseAmount, buffer);
            SendMessage(g_hSlider, TBM_SETPOS, TRUE, raiseAmount);

            // Frontend-only for now.
            // Later, this raiseAmount can be sent to the actual game/client logic.
            ShowRaiseControls(false);

            break;
        }

        case ID_BTN_CANCEL_RAISE:
            ShowRaiseControls(false);
            break;

        case ID_BTN_CALL:
        case ID_BTN_CHECK:
        case ID_BTN_FOLD:
            ShowRaiseControls(false);
            break;
        }
        break;

    case WM_HSCROLL:
        if ((HWND)lParam == g_hSlider)
        {
            UpdateRaiseAmountText();
        }
        break;

    case WM_PAINT:
    {
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
