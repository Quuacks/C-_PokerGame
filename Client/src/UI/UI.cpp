#include "../framework.h"
#include "UI.h"
#include "../Network/NetworkClient.h" // Points to your client wrapper
#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <memory>
#include <objidl.h>
#include <gdiplus.h>
#include <json.hpp>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")

using json = nlohmann::json;

#define ID_BTN_RAISE          1001
#define ID_BTN_CALL           1002
#define ID_BTN_CHECK          1003
#define ID_BTN_FOLD           1004
#define ID_BTN_START_GAME     1005

#define ID_SLIDER             2001
#define ID_EDIT_RAISE_AMOUNT  2002
#define ID_BTN_CONFIRM_RAISE  2003
#define ID_BTN_CANCEL_RAISE   2004

#define ID_COMBO_HAND         3001
#define ID_LIST_PLAYERS       3002
#define ID_STATUS_LOG         3003

#define ID_TIMER_TURN_POPUP   4001

// Global handles required for Win32 callbacks
HINSTANCE hInst;
HWND g_hSlider;
HWND g_editRaiseAmount;
HWND g_btnConfirmRaise;
HWND g_btnCancelRaise;
HWND g_btnRaise, g_btnCall, g_btnCheck, g_btnFold;
HWND g_btnStartGame;
HWND g_comboHands, g_listPlayers;
HWND g_statusLog;

bool g_isHeroTurn = false;
bool g_showTurnPopup = false;

HWND g_mainHWnd = nullptr;

const int MIN_RAISE = 0;
const int MAX_RAISE = 10000; // Increased to match typical chip stacks
const int DEFAULT_PLAYER_CHIPS = 1500;

int g_potAmount = 0;
std::vector<Card> g_boardCards;
std::vector<Card> g_heroCards;
std::vector<std::wstring> g_players;
std::vector<int> g_playerChips;
int g_heroChips = DEFAULT_PLAYER_CHIPS;
int g_selectedComboIndex = 0;

bool g_raiseControlsVisible = false;
int g_raisePreviewAmount = 10;

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

ULONG_PTR g_gdiplusToken = 0;
std::unique_ptr<Gdiplus::Image> g_chipSheet;

const int CHIP_SPRITE_WIDTH = 64;
const int CHIP_SPRITE_HEIGHT = 72;
const int CHIP_SHEET_COLUMNS = 5;

struct ChipDenomination
{
    int value;
    int spriteIndex;
};

const ChipDenomination CHIP_DENOMS[] = {
    {500, 0},
    {100, 1},
    {25, 2},
    {5, 3},
    {1, 4}
};

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

bool IsActionButtonId(int controlId)
{
    return controlId == ID_BTN_RAISE ||
        controlId == ID_BTN_CALL ||
        controlId == ID_BTN_CHECK ||
        controlId == ID_BTN_FOLD;
}

void RefreshActionButtons()
{
    HWND buttons[] = {
        g_btnRaise,
        g_btnCall,
        g_btnCheck,
        g_btnFold
    };

    for (HWND button : buttons)
    {
        if (button)
            InvalidateRect(button, nullptr, TRUE);
    }

    if (g_mainHWnd)
        InvalidateRect(g_mainHWnd, nullptr, TRUE);
}

void SetActionButtonsEnabled(bool enabled)
{
    HWND buttons[] = {
        g_btnRaise,
        g_btnCall,
        g_btnCheck,
        g_btnFold
    };

    for (HWND button : buttons)
    {
        if (button)
            EnableWindow(button, enabled ? TRUE : FALSE);
    }

    if (!enabled)
    {
        ShowRaiseControls(false);
    }

    RefreshActionButtons();
}

void ShowYourTurnPopup()
{
    if (!g_mainHWnd)
        return;

    g_showTurnPopup = true;

    SetTimer(
        g_mainHWnd,
        ID_TIMER_TURN_POPUP,
        3000,
        nullptr
    );

    AddStatusMessage(L"Your turn.");
    InvalidateRect(g_mainHWnd, nullptr, TRUE);
}

void SetHeroTurn(bool isHeroTurn)
{
    bool wasHeroTurn = g_isHeroTurn;
    g_isHeroTurn = isHeroTurn;

    SetActionButtonsEnabled(g_isHeroTurn);

    if (g_isHeroTurn && !wasHeroTurn)
    {
        ShowYourTurnPopup();
    }
    else if (!g_isHeroTurn)
    {
        g_showTurnPopup = false;

        if (g_mainHWnd)
            KillTimer(g_mainHWnd, ID_TIMER_TURN_POPUP);
    }

    RefreshActionButtons();
}

void DrawTurnPopup(HDC hdc, const RECT& clientRect)
{
    if (!g_showTurnPopup)
        return;

    const int boxWidth = 300;
    const int boxHeight = 70;

    int centerX = (clientRect.left + clientRect.right) / 2;

    RECT boxRect = {
        centerX - boxWidth / 2,
        clientRect.top + 70,
        centerX + boxWidth / 2,
        clientRect.top + 70 + boxHeight
    };

    HBRUSH popupBrush = CreateSolidBrush(RGB(255, 215, 80));
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, popupBrush);

    HPEN borderPen = CreatePen(PS_SOLID, 3, RGB(120, 80, 0));
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);

    RoundRect(
        hdc,
        boxRect.left,
        boxRect.top,
        boxRect.right,
        boxRect.bottom,
        18,
        18
    );

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(popupBrush);
    DeleteObject(borderPen);

    SetBkMode(hdc, TRANSPARENT);

    HFONT popupFont = CreateFontW(
        30,
        0,
        0,
        0,
        FW_BOLD,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        L"Segoe UI"
    );

    HFONT oldFont = (HFONT)SelectObject(hdc, popupFont);
    COLORREF oldTextColor = SetTextColor(hdc, RGB(20, 20, 20));

    DrawText(
        hdc,
        L"YOUR TURN",
        -1,
        &boxRect,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );

    SetTextColor(hdc, oldTextColor);
    SelectObject(hdc, oldFont);
    DeleteObject(popupFont);
}

void DrawActionButton(const DRAWITEMSTRUCT* drawInfo)
{
    if (!drawInfo)
        return;

    HDC hdc = drawInfo->hDC;
    RECT rc = drawInfo->rcItem;

    bool pressed = (drawInfo->itemState & ODS_SELECTED) != 0;
    bool focused = (drawInfo->itemState & ODS_FOCUS) != 0;
    bool disabled = (drawInfo->itemState & ODS_DISABLED) != 0;

    COLORREF backgroundColor;
    COLORREF borderColor;
    COLORREF textColor;

    if (disabled)
    {
        backgroundColor = RGB(210, 210, 210);
        borderColor = RGB(160, 160, 160);
        textColor = RGB(120, 120, 120);
    }
    else if (g_isHeroTurn)
    {
        backgroundColor = pressed ? RGB(220, 170, 40) : RGB(255, 210, 70);
        borderColor = RGB(130, 90, 0);
        textColor = RGB(20, 20, 20);
    }
    else
    {
        backgroundColor = pressed ? RGB(190, 190, 190) : RGB(235, 235, 235);
        borderColor = RGB(120, 120, 120);
        textColor = RGB(0, 0, 0);
    }

    HBRUSH buttonBrush = CreateSolidBrush(backgroundColor);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, buttonBrush);

    HPEN borderPen = CreatePen(
        PS_SOLID,
        g_isHeroTurn ? 3 : 1,
        borderColor
    );

    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);

    RoundRect(
        hdc,
        rc.left,
        rc.top,
        rc.right,
        rc.bottom,
        12,
        12
    );

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(buttonBrush);
    DeleteObject(borderPen);

    wchar_t text[64];
    GetWindowTextW(drawInfo->hwndItem, text, 64);

    SetBkMode(hdc, TRANSPARENT);
    COLORREF oldTextColor = SetTextColor(hdc, textColor);

    HFONT buttonFont = CreateFontW(
        20,
        0,
        0,
        0,
        g_isHeroTurn ? FW_BOLD : FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        L"Segoe UI"
    );

    HFONT oldFont = (HFONT)SelectObject(hdc, buttonFont);

    DrawText(
        hdc,
        text,
        -1,
        &rc,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );

    if (focused)
    {
        RECT focusRect = rc;
        InflateRect(&focusRect, -5, -5);
        DrawFocusRect(hdc, &focusRect);
    }

    SelectObject(hdc, oldFont);
    DeleteObject(buttonFont);

    SetTextColor(hdc, oldTextColor);
}

std::wstring ResolveExistingAssetPath(const std::vector<std::wstring>& candidates)
{
    for (const auto& candidate : candidates)
    {
        if (GetFileAttributesW(candidate.c_str()) != INVALID_FILE_ATTRIBUTES)
            return candidate;
    }

    return candidates.empty() ? L"" : candidates.front();
}

void StartGdiPlus()
{
    if (g_gdiplusToken != 0)
        return;

    Gdiplus::GdiplusStartupInput startupInput;
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &startupInput, nullptr);
}

void ShutdownGdiPlus()
{
    g_chipSheet.reset();

    if (g_gdiplusToken != 0)
    {
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
}

void LoadChipAssets()
{
    std::wstring chipPath = ResolveExistingAssetPath({
        L"src\\Textures\\Chips\\Top-Down\\Chips\\ChipsA_Flat-64x72.png",
        L"..\\Client\\src\\Textures\\Chips\\Top-Down\\Chips\\ChipsA_Flat-64x72.png",
        L"assets\\chips\\ChipsA_Flat-64x72.png"
        });

    g_chipSheet = std::make_unique<Gdiplus::Image>(chipPath.c_str());

    if (g_chipSheet && g_chipSheet->GetLastStatus() == Gdiplus::Ok)
        AddStatusMessage(L"Chip assets loaded.");
    else
        AddStatusMessage(L"Chip asset not found. Using fallback chip drawing.");
}

void EnsurePlayerChipDefaults()
{
    if (g_playerChips.size() < g_players.size())
        g_playerChips.resize(g_players.size(), DEFAULT_PLAYER_CHIPS);
}

int GetPlayerChipAmount(int playerIndex)
{
    if (playerIndex >= 0 && playerIndex < (int)g_playerChips.size())
        return g_playerChips[playerIndex];

    return DEFAULT_PLAYER_CHIPS;
}

void DrawChipSprite(Gdiplus::Graphics& graphics, int x, int y, int width, int height, int spriteIndex)
{
    if (g_chipSheet && g_chipSheet->GetLastStatus() == Gdiplus::Ok)
    {
        int col = spriteIndex % CHIP_SHEET_COLUMNS;
        int row = spriteIndex / CHIP_SHEET_COLUMNS;

        graphics.DrawImage(
            g_chipSheet.get(),
            Gdiplus::Rect(x, y, width, height),
            col * CHIP_SPRITE_WIDTH,
            row * CHIP_SPRITE_HEIGHT,
            CHIP_SPRITE_WIDTH,
            CHIP_SPRITE_HEIGHT,
            Gdiplus::UnitPixel
        );

        return;
    }

    Gdiplus::SolidBrush fallbackBrush(Gdiplus::Color(255, 220, 220, 220));
    Gdiplus::Pen fallbackPen(Gdiplus::Color(255, 60, 60, 60), 2.0f);

    graphics.FillEllipse(&fallbackBrush, x, y + height / 4, width, height / 2);
    graphics.DrawEllipse(&fallbackPen, x, y + height / 4, width, height / 2);
}

void DrawChipStack(HDC hdc, Gdiplus::Graphics& graphics, int x, int y, int amount, const std::wstring& label)
{
    const int chipW = 34;
    const int chipH = 38;
    const int stackGap = 24;
    const int chipVerticalOffset = 5;
    const int maxChipsPerDenom = 5;

    int remaining = (std::max)(0, amount);
    int stackIndex = 0;

    for (const auto& denom : CHIP_DENOMS)
    {
        if (remaining <= 0)
            break;

        int chipCount = remaining / denom.value;
        remaining %= denom.value;

        if (chipCount <= 0)
            continue;

        chipCount = (std::min)(chipCount, maxChipsPerDenom);

        int stackX = x + stackIndex * stackGap;

        for (int i = 0; i < chipCount; ++i)
        {
            int chipY = y - i * chipVerticalOffset;
            DrawChipSprite(graphics, stackX, chipY, chipW, chipH, denom.spriteIndex);
        }

        stackIndex++;
    }

    if (amount <= 0)
    {
        DrawChipSprite(graphics, x, y, chipW, chipH, 4);
    }

    SetBkMode(hdc, TRANSPARENT);
    COLORREF previousColor = SetTextColor(hdc, RGB(255, 255, 255));

    if (!label.empty())
    {
        wchar_t amountText[128];
        wsprintf(amountText, L"%s: %d", label.c_str(), amount);

        RECT textRect = {
            x - 35,
            y + chipH + 2,
            x + 155,
            y + chipH + 28
        };

        DrawText(hdc, amountText, -1, &textRect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
    }

    SetTextColor(hdc, previousColor);
}

struct PlayerSeatLayout
{
    RECT nameBox;
    POINT chipPosition;
};

PlayerSeatLayout GetPlayerSeatLayout(int playerIndex, int playerCount, const RECT& tableRect)
{
    int centerX = (tableRect.left + tableRect.right) / 2;
    int centerY = (tableRect.top + tableRect.bottom) / 2;

    const int nameBoxWidth = 130;
    const int nameBoxHeight = 32;

    PlayerSeatLayout seats[6] = {
        // 0: Hero / bottom center
        {
            { centerX - nameBoxWidth / 2, tableRect.bottom - 90, centerX + nameBoxWidth / 2, tableRect.bottom - 58 },
            { centerX - 45, tableRect.bottom - 160 }
        },

        // 1: Bottom left
        {
            { tableRect.left + 45, tableRect.bottom - 145, tableRect.left + 45 + nameBoxWidth, tableRect.bottom - 113 },
            { tableRect.left + 95, tableRect.bottom - 210 }
        },

        // 2: Top left
        {
            { tableRect.left + 80, tableRect.top + 45, tableRect.left + 80 + nameBoxWidth, tableRect.top + 77 },
            { tableRect.left + 130, tableRect.top + 100 }
        },

        // 3: Top center
        {
            { centerX - nameBoxWidth / 2, tableRect.top + 35, centerX + nameBoxWidth / 2, tableRect.top + 67 },
            { centerX - 45, tableRect.top + 95 }
        },

        // 4: Top right
        {
            { tableRect.right - 80 - nameBoxWidth, tableRect.top + 45, tableRect.right - 80, tableRect.top + 77 },
            { tableRect.right - 200, tableRect.top + 100 }
        },

        // 5: Bottom right
        {
            { tableRect.right - 45 - nameBoxWidth, tableRect.bottom - 145, tableRect.right - 45, tableRect.bottom - 113 },
            { tableRect.right - 190, tableRect.bottom - 210 }
        }
    };

    return seats[playerIndex % 6];
}

void DrawPlayerNameBox(HDC hdc, const RECT& nameBox, const std::wstring& playerName, int chipAmount)
{
    HBRUSH boxBrush = CreateSolidBrush(RGB(20, 65, 20));
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, boxBrush);

    HPEN borderPen = CreatePen(PS_SOLID, 2, RGB(230, 230, 230));
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);

    RoundRect(
        hdc,
        nameBox.left,
        nameBox.top,
        nameBox.right,
        nameBox.bottom,
        12,
        12
    );

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(boxBrush);
    DeleteObject(borderPen);

    SetBkMode(hdc, TRANSPARENT);
    COLORREF oldColor = SetTextColor(hdc, RGB(255, 255, 255));

    RECT nameTextRect = {
        nameBox.left + 6,
        nameBox.top + 2,
        nameBox.right - 6,
        nameBox.top + 17
    };

    DrawText(
        hdc,
        playerName.c_str(),
        -1,
        &nameTextRect,
        DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS
    );

    wchar_t chipText[64];
    wsprintf(chipText, L"%d chips", chipAmount);

    RECT chipTextRect = {
        nameBox.left + 6,
        nameBox.top + 16,
        nameBox.right - 6,
        nameBox.bottom - 2
    };

    DrawText(
        hdc,
        chipText,
        -1,
        &chipTextRect,
        DT_CENTER | DT_SINGLELINE | DT_VCENTER
    );

    SetTextColor(hdc, oldColor);
}

void DrawPlayerChipStacks(HDC hdc, Gdiplus::Graphics& graphics, const RECT& tableRect)
{
    EnsurePlayerChipDefaults();

    int playerCount = (int)g_players.size();

    if (playerCount == 0)
    {
        return;
    }

    int playersToDraw = (std::min)(playerCount, 6);

    for (int i = 0; i < playersToDraw; ++i)
    {
        int chipAmount = GetPlayerChipAmount(i);
        PlayerSeatLayout seat = GetPlayerSeatLayout(i, playersToDraw, tableRect);

        DrawPlayerNameBox(hdc, seat.nameBox, g_players[i], chipAmount);

        DrawChipStack(
            hdc,
            graphics,
            seat.chipPosition.x,
            seat.chipPosition.y,
            chipAmount,
            L""
        );
    }
}

void DrawMainPot(HDC hdc, Gdiplus::Graphics& graphics, const RECT& tableRect)
{
    int centerX = (tableRect.left + tableRect.right) / 2;
    int centerY = (tableRect.top + tableRect.bottom) / 2;

    DrawChipStack(hdc, graphics, centerX - 55, centerY - 35, g_potAmount, L"Main Pot");
}

void DrawRaisePreview(HDC hdc, Gdiplus::Graphics& graphics)
{
    if (!g_raiseControlsVisible || !g_hSlider || !g_mainHWnd)
        return;

    RECT sliderRect;
    GetWindowRect(g_hSlider, &sliderRect);
    MapWindowPoints(nullptr, g_mainHWnd, (POINT*)&sliderRect, 2);

    int previewX = sliderRect.left + ((sliderRect.right - sliderRect.left) / 2) - 45;
    int previewY = sliderRect.top - 60;

    if (previewY < 5)
        previewY = sliderRect.bottom + 8;

    DrawChipStack(hdc, graphics, previewX, previewY, g_raisePreviewAmount, L"Raise Preview");
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

bool SendPlayerActionToServer(const std::string& actionType, json data = json::object())
{
    if (!g_NetworkClient)
    {
        AddStatusMessage(L"Cannot send action: client is not connected.");
        return false;
    }

    if (!data.is_object())
        data = json::object();

    // Include the action type both at the packet level and inside data.
    // This keeps it compatible with the current server handler style.
    data["type"] = actionType;

    json packet = {
        {"type", actionType},
        {"data", data}
    };

    bool sent = g_NetworkClient->SendRequest(packet.dump() + "\n");

    if (sent)
        AddStatusMessageFromUtf8("Sent " + actionType + " request.");
    else
        AddStatusMessageFromUtf8("Failed to send " + actionType + " request.");

    return sent;
}

void ShowRaiseControls(bool show) {
    g_raiseControlsVisible = show;

    int command = show ? SW_SHOW : SW_HIDE;

    ShowWindow(g_hSlider, command);
    ShowWindow(g_editRaiseAmount, command);
    ShowWindow(g_btnConfirmRaise, command);
    ShowWindow(g_btnCancelRaise, command);

    if (g_mainHWnd)
        InvalidateRect(g_mainHWnd, nullptr, TRUE);
}

void UpdateRaiseAmountText() {
    int amount = (int)SendMessage(g_hSlider, TBM_GETPOS, 0, 0);
    g_raisePreviewAmount = amount;

    wchar_t buffer[32];
    wsprintf(buffer, L"%d", amount);

    SetWindowText(g_editRaiseAmount, buffer);

    if (g_mainHWnd)
        InvalidateRect(g_mainHWnd, nullptr, TRUE);
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

    MoveWindow(
        g_btnStartGame,
        margin + 4 * (btnWidth + 10),
        yButtons,
        btnWidth + 30,
        btnHeight,
        TRUE
    );

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
    g_playerChips = state.playerChips;
    g_heroChips = state.heroChips;
    g_selectedComboIndex = state.selectedComboIndex;

    EnsurePlayerChipDefaults();

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

    Gdiplus::Graphics graphics(hdc);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

    wchar_t potBuf[64];
    wsprintf(potBuf, L"Pot: %d", g_potAmount);
    SetBkMode(hdc, TRANSPARENT);
    RECT potRect = { (w / 2) - 100, rc.top + 10, (w / 2) + 100, rc.top + 40 };
    DrawText(hdc, potBuf, -1, &potRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    DrawPlayerChipStacks(hdc, graphics, center);
    DrawMainPot(hdc, graphics, center);
    DrawRaisePreview(hdc, graphics);

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
        case ID_BTN_START_GAME:
            AddStatusMessage(L"Start game selected.");
            SendPlayerActionToServer("START_GAME");
            break;

        case ID_BTN_RAISE:
            if (!g_isHeroTurn)
            {
                AddStatusMessage(L"Not your turn.");
                break;
            }

            ShowRaiseControls(true);
            UpdateRaiseAmountText();
            AddStatusMessage(L"Raise selected. Choose an amount.");
            break;

        case ID_EDIT_RAISE_AMOUNT:
            if (HIWORD(wParam) == EN_CHANGE) {
                int amount = GetRaiseAmountFromEdit();
                g_raisePreviewAmount = amount;
                SendMessage(g_hSlider, TBM_SETPOS, TRUE, amount);
                InvalidateRect(hWnd, nullptr, TRUE);
            }
            break;

        case ID_BTN_CONFIRM_RAISE: {
            if (!g_isHeroTurn)
            {
                ShowRaiseControls(false);
                AddStatusMessage(L"Not your turn.");
                break;
            }

            int raiseAmount = GetRaiseAmountFromEdit();
            g_raisePreviewAmount = raiseAmount;
            ShowRaiseControls(false);

            wchar_t statusBuffer[128];
            wsprintf(statusBuffer, L"Raise amount selected: %d", raiseAmount);
            AddStatusMessage(statusBuffer);

            json data = {
                {"amount", raiseAmount}
            };

            SendPlayerActionToServer("RAISE", data);
            break;
        }

        case ID_BTN_CANCEL_RAISE:
            ShowRaiseControls(false);
            AddStatusMessage(L"Raise cancelled.");
            break;

        case ID_BTN_CALL:
            if (!g_isHeroTurn)
            {
                AddStatusMessage(L"Not your turn.");
                break;
            }

            ShowRaiseControls(false);
            AddStatusMessage(L"Call selected.");

            SendPlayerActionToServer("CALL");
            
            break;

        case ID_BTN_CHECK:
            if (!g_isHeroTurn)
            {
                AddStatusMessage(L"Not your turn.");
                break;
            }

            ShowRaiseControls(false);
            AddStatusMessage(L"Check selected.");
            
            SendPlayerActionToServer("CHECK");

            break;

        case ID_BTN_FOLD:
            if (!g_isHeroTurn)
            {
                AddStatusMessage(L"Not your turn.");
                break;
            }

            ShowRaiseControls(false);
            AddStatusMessage(L"Fold selected.");

            SendPlayerActionToServer("FOLD");
            break;
        }
        break;

    case WM_HSCROLL:
        if ((HWND)lParam == g_hSlider) {
            UpdateRaiseAmountText();
        }
        break;

    case WM_DRAWITEM:
    {
        int controlId = (int)wParam;

        if (IsActionButtonId(controlId))
        {
            DrawActionButton((DRAWITEMSTRUCT*)lParam);
            return TRUE;
        }

        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);

        DrawTable(hdc, rc);
        DrawTurnPopup(hdc, rc);

        EndPaint(hWnd, &ps);
    }
                 break;
    case WM_TIMER:
        if (wParam == ID_TIMER_TURN_POPUP)
        {
            KillTimer(hWnd, ID_TIMER_TURN_POPUP);
            g_showTurnPopup = false;
            InvalidateRect(hWnd, nullptr, TRUE);
            return 0;
        }
        break;

    case WM_DESTROY:
        KillTimer(hWnd, ID_TIMER_TURN_POPUP);
        ShutdownGdiPlus();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
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

    g_mainHWnd = hWnd;
    StartGdiPlus();

    g_btnRaise = CreateWindow(L"BUTTON", L"Raise", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hWnd, (HMENU)ID_BTN_RAISE, hInst, nullptr);
    g_btnCall = CreateWindow(L"BUTTON", L"Call", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hWnd, (HMENU)ID_BTN_CALL, hInst, nullptr);
    g_btnCheck = CreateWindow(L"BUTTON", L"Check", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hWnd, (HMENU)ID_BTN_CHECK, hInst, nullptr);
    g_btnFold = CreateWindow(L"BUTTON", L"Fold", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 0, 0, 0, 0, hWnd, (HMENU)ID_BTN_FOLD, hInst, nullptr);    g_hSlider = CreateWindowEx(0, TRACKBAR_CLASS, L"", WS_CHILD | TBS_AUTOTICKS, 0, 0, 0, 0, hWnd, (HMENU)ID_SLIDER, hInst, nullptr);

    g_btnStartGame = CreateWindow(
        L"BUTTON",
        L"Start Game",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 0, 0,
        hWnd,
        (HMENU)ID_BTN_START_GAME,
        hInst,
        nullptr
    );

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

    SetHeroTurn(false);

    ShowWindow(hWnd, nCmdShow);
    LoadCardAssets();
    LoadChipAssets();
    UpdateWindow(hWnd);

    return TRUE;
}
