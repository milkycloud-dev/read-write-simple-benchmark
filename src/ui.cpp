#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <windowsx.h>
#include <objidl.h>
#include <gdiplus.h>
#include <dwmapi.h>
#include <shlobj.h>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>

#include "ui.hpp"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "shell32.lib")

using namespace Gdiplus;

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

static void DrawRoundedRect(Graphics& g, Pen& pen, RectF rect, float radius) {
    float x = rect.X;
    float y = rect.Y;
    float w = rect.Width;
    float h = rect.Height;
    float d = radius * 2.0f;

    GraphicsPath path;
    path.AddArc(x, y, d, d, 180.0f, 90.0f);
    path.AddArc(x + w - d, y, d, d, 270.0f, 90.0f);
    path.AddArc(x + w - d, y + h - d, d, d, 0.0f, 90.0f);
    path.AddArc(x, y + h - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();
    g.DrawPath(&pen, &path);
}

static void FillRoundedRect(Graphics& g, Brush& brush, RectF rect, float radius) {
    float x = rect.X;
    float y = rect.Y;
    float w = rect.Width;
    float h = rect.Height;
    float d = radius * 2.0f;

    GraphicsPath path;
    path.AddArc(x, y, d, d, 180.0f, 90.0f);
    path.AddArc(x + w - d, y, d, d, 270.0f, 90.0f);
    path.AddArc(x + w - d, y + h - d, d, d, 0.0f, 90.0f);
    path.AddArc(x, y + h - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();
    g.FillPath(&brush, &path);
}

MainWindow::MainWindow(BenchmarkEngine& engine)
    : m_engine(engine) {}

MainWindow::~MainWindow() {
    CleanupMemBuffer();
}

bool MainWindow::Create() {
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = MainWindow::WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"RWBenchMinimalistWindowClass";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.style = CS_HREDRAW | CS_VREDRAW;

    RegisterClassExW(&wc);

    int clientW = 780;
    int clientH = 580;

    RECT rc = { 0, 0, clientW, clientH };
    AdjustWindowRectEx(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE, 0);

    int winW = rc.right - rc.left;
    int winH = rc.bottom - rc.top;
    int posX = (GetSystemMetrics(SM_CXSCREEN) - winW) / 2;
    int posY = (GetSystemMetrics(SM_CYSCREEN) - winH) / 2;

    m_hWnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"RW-BENCH \u2022 Disk Speed Benchmark",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY, winW, winH,
        nullptr, nullptr, wc.hInstance, this
    );

    if (!m_hWnd) return false;

    // Windows 10/11 Dark Titlebar
    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(m_hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));

    PopulateDrives();

    // 30 FPS timer for live rendering
    SetTimer(m_hWnd, 1001, 33, nullptr);

    return true;
}

void MainWindow::Show(int nCmdShow) {
    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);
}

void MainWindow::PopulateDrives() {
    m_drives = DetectAllDrives();
    if (!m_drives.empty()) {
        m_selectedDriveIndex = 0;
    }
}

int MainWindow::HitTest(int x, int y) const {
    // 1: Drive Selector Pill (x: 446..756, y: 16..50)
    if (x >= 446 && x <= 756 && y >= 16 && y <= 50) {
        return 1;
    }

    // Mode Pills (y: 62..94)
    if (y >= 62 && y <= 94) {
        if (x >= 24 && x <= 108)  return 10; // Full Suite (w=84)
        if (x >= 114 && x <= 194) return 11; // Seq Write  (w=80)
        if (x >= 200 && x <= 280) return 12; // Seq Read   (w=80)
        if (x >= 286 && x <= 374) return 13; // Random 4K  (w=88)
    }

    // Size Pills (y: 62..94)
    if (y >= 62 && y <= 94) {
        if (x >= 390 && x <= 442) return 20; // 128 MB (w=52)
        if (x >= 447 && x <= 499) return 21; // 256 MB (w=52)
        if (x >= 504 && x <= 556) return 22; // 512 MB (w=52)
        if (x >= 561 && x <= 611) return 23; // 1 GB   (w=50)
    }

    // 30: Start / Stop Action Button (x: 626..756, y: 60..94)
    if (x >= 626 && x <= 756 && y >= 60 && y <= 94) {
        return 30;
    }

    return 0;
}

void MainWindow::ShowDriveMenu() {
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    for (size_t i = 0; i < m_drives.size(); ++i) {
        UINT flags = MF_STRING;
        if (static_cast<int>(i) == m_selectedDriveIndex) {
            flags |= MF_CHECKED;
        }
        AppendMenuW(hMenu, flags, 1000 + i, m_drives[i].GetDisplayName().c_str());
    }

    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, 1999, L"Browse Custom Folder...");

    POINT pt = { 446, 52 };
    ClientToScreen(m_hWnd, &pt);

    int cmd = TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x, pt.y, m_hWnd, nullptr);
    DestroyMenu(hMenu);

    if (cmd >= 1000 && cmd < 1000 + static_cast<int>(m_drives.size())) {
        m_selectedDriveIndex = cmd - 1000;
        InvalidateRect(m_hWnd, nullptr, FALSE);
    } else if (cmd == 1999) {
        PromptCustomDirectory();
    }
}

void MainWindow::PromptCustomDirectory() {
    BROWSEINFOW bi = { 0 };
    bi.hwndOwner = m_hWnd;
    bi.lpszTitle = L"Select Storage Directory or Drive";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        wchar_t path[MAX_PATH] = { 0 };
        if (SHGetPathFromIDListW(pidl, path)) {
            m_customPath = path;
            if (m_customPath.back() != L'\\') m_customPath += L'\\';

            DriveInfo customInfo;
            customInfo.path = m_customPath;
            customInfo.volumeName = L"Custom Folder";
            customInfo.fileSystem = L"Directory";
            customInfo.driveType = L"Folder";

            ULARGE_INTEGER freeBytes = { 0 }, totalBytes = { 0 };
            if (GetDiskFreeSpaceExW(m_customPath.c_str(), &freeBytes, &totalBytes, nullptr)) {
                customInfo.freeBytes = freeBytes.QuadPart;
                customInfo.totalBytes = totalBytes.QuadPart;
            }
            m_drives.push_back(customInfo);
            m_selectedDriveIndex = static_cast<int>(m_drives.size() - 1);
            InvalidateRect(m_hWnd, nullptr, FALSE);
        }
        CoTaskMemFree(pidl);
    }
}

void MainWindow::HandleStartStop() {
    LiveStatus s = m_engine.GetStatus();
    if (s.isRunning) {
        m_engine.Abort();
        return;
    }

    if (m_selectedDriveIndex < 0 || m_selectedDriveIndex >= static_cast<int>(m_drives.size())) {
        MessageBoxW(m_hWnd, L"Please select a storage drive first.", L"RW-Bench", MB_ICONWARNING);
        return;
    }

    std::wstring targetPath = m_drives[m_selectedDriveIndex].path;

    TestMode mode = TestMode::FullSuite;
    if (m_selectedMode == 1) mode = TestMode::SequentialWrite;
    else if (m_selectedMode == 2) mode = TestMode::SequentialRead;
    else if (m_selectedMode == 3) mode = TestMode::Random4KWrite;

    uint64_t targetSize = 256ULL * 1024ULL * 1024ULL;
    if (m_selectedSize == 0) targetSize = 128ULL * 1024ULL * 1024ULL;
    else if (m_selectedSize == 1) targetSize = 256ULL * 1024ULL * 1024ULL;
    else if (m_selectedSize == 2) targetSize = 512ULL * 1024ULL * 1024ULL;
    else if (m_selectedSize == 3) targetSize = 1024ULL * 1024ULL * 1024ULL;

    m_engine.Start(targetPath, mode, targetSize, 1048576);
}

void MainWindow::CreateMemBuffer(int width, int height) {
    if (m_hMemDC && m_memWidth == width && m_memHeight == height) {
        return;
    }
    CleanupMemBuffer();

    HDC hdc = GetDC(m_hWnd);
    m_hMemDC = CreateCompatibleDC(hdc);
    m_hMemBitmap = CreateCompatibleBitmap(hdc, width, height);
    m_hOldBitmap = reinterpret_cast<HBITMAP>(SelectObject(m_hMemDC, m_hMemBitmap));
    m_memWidth = width;
    m_memHeight = height;
    ReleaseDC(m_hWnd, hdc);
}

void MainWindow::CleanupMemBuffer() {
    if (m_hMemDC) {
        SelectObject(m_hMemDC, m_hOldBitmap);
        DeleteObject(m_hMemBitmap);
        DeleteDC(m_hMemDC);
        m_hMemDC = nullptr;
        m_hMemBitmap = nullptr;
    }
}

LRESULT CALLBACK MainWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hWnd = hWnd;
    } else {
        pThis = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(msg, wParam, lParam);
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(m_hWnd, &ps);
            OnPaint();
            EndPaint(m_hWnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_TIMER:
            OnTimer();
            return 0;

        case WM_LBUTTONDOWN: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            int hit = HitTest(x, y);
            LiveStatus s = m_engine.GetStatus();

            if (hit == 1) {
                if (!s.isRunning) ShowDriveMenu();
            } else if (hit >= 10 && hit <= 13) {
                if (!s.isRunning) {
                    m_selectedMode = hit - 10;
                    InvalidateRect(m_hWnd, nullptr, FALSE);
                }
            } else if (hit >= 20 && hit <= 23) {
                if (!s.isRunning) {
                    m_selectedSize = hit - 20;
                    InvalidateRect(m_hWnd, nullptr, FALSE);
                }
            } else if (hit == 30) {
                HandleStartStop();
            }
            return 0;
        }

        case WM_MOUSEMOVE: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            int hit = HitTest(x, y);

            if (!m_trackingMouse) {
                TRACKMOUSEEVENT tme = { sizeof(tme) };
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = m_hWnd;
                TrackMouseEvent(&tme);
                m_trackingMouse = true;
            }

            if (hit != m_hoverId) {
                m_hoverId = hit;
                InvalidateRect(m_hWnd, nullptr, FALSE);
            }
            return 0;
        }

        case WM_MOUSELEAVE: {
            m_trackingMouse = false;
            if (m_hoverId != 0) {
                m_hoverId = 0;
                InvalidateRect(m_hWnd, nullptr, FALSE);
            }
            return 0;
        }

        case WM_SETCURSOR: {
            if (LOWORD(lParam) == HTCLIENT && m_hoverId != 0) {
                SetCursor(LoadCursorW(nullptr, IDC_HAND));
                return TRUE;
            }
            break;
        }

        case WM_KEYDOWN: {
            if (wParam == VK_SPACE || wParam == VK_RETURN) {
                HandleStartStop();
                return 0;
            } else if (wParam == VK_ESCAPE) {
                if (m_engine.GetStatus().isRunning) {
                    m_engine.Abort();
                    return 0;
                }
            }
            break;
        }

        case WM_DESTROY:
            OnDestroy();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(m_hWnd, msg, wParam, lParam);
}

void MainWindow::OnTimer() {
    InvalidateRect(m_hWnd, nullptr, FALSE);
}

void MainWindow::OnDestroy() {
    KillTimer(m_hWnd, 1001);
    m_engine.Abort();
}

void MainWindow::OnPaint() {
    RECT rc;
    GetClientRect(m_hWnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    CreateMemBuffer(width, height);

    Graphics g(m_hMemDC);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    g.SetPixelOffsetMode(PixelOffsetModeHalf);

    // Deep Dark Minimalist Canvas (#0D1117)
    SolidBrush brushCanvas(Color(255, 13, 17, 23));
    g.FillRectangle(&brushCanvas, 0, 0, width, height);

    // Theme Colors
    Color colorCard(255, 22, 27, 34);         // #161B22
    Color colorBorder(255, 48, 54, 61);       // #30363D
    Color colorPillHover(255, 34, 42, 53);
    Color colorActiveBlue(255, 88, 166, 255); // #58A6FF
    Color colorEmerald(255, 63, 185, 80);     // #3FB950
    Color colorRose(255, 248, 81, 73);        // #F85149
    Color colorTextWhite(255, 240, 246, 252); // #F0F6FC
    Color colorTextMuted(255, 139, 148, 158); // #8B949E
    Color colorTextDim(255, 88, 96, 105);

    SolidBrush brushCard(colorCard);
    Pen penCard(colorBorder, 1.0f);
    Pen penDivider(Color(255, 33, 38, 45), 1.0f);

    SolidBrush brushWhite(colorTextWhite);
    SolidBrush brushMuted(colorTextMuted);
    SolidBrush brushDim(colorTextDim);
    SolidBrush brushCyan(colorActiveBlue);
    SolidBrush brushEmerald(colorEmerald);

    // Native Segoe UI Fonts
    Font fontBrand(L"Segoe UI", 12.0f, FontStyleBold, UnitPoint);
    Font fontSubBrand(L"Segoe UI", 8.5f, FontStyleRegular, UnitPoint);
    Font fontPill(L"Segoe UI", 9.0f, FontStyleRegular, UnitPoint);
    Font fontPillBold(L"Segoe UI", 9.0f, FontStyleBold, UnitPoint);
    Font fontSpeed(L"Segoe UI", 42.0f, FontStyleBold, UnitPoint);
    Font fontSpeedUnit(L"Segoe UI", 13.0f, FontStyleBold, UnitPoint);
    Font fontStatus(L"Segoe UI", 9.5f, FontStyleBold, UnitPoint);
    Font fontStatLabel(L"Segoe UI", 9.0f, FontStyleRegular, UnitPoint);
    Font fontStatValue(L"Segoe UI", 9.0f, FontStyleBold, UnitPoint);
    Font fontCardTitle(L"Segoe UI", 8.5f, FontStyleBold, UnitPoint);
    Font fontColLabel(L"Segoe UI", 8.0f, FontStyleBold, UnitPoint);
    Font fontColValue(L"Segoe UI", 17.0f, FontStyleBold, UnitPoint);
    Font fontColSub(L"Segoe UI", 8.5f, FontStyleRegular, UnitPoint);
    Font fontBtn(L"Segoe UI", 10.5f, FontStyleBold, UnitPoint);

    StringFormat sfCenter;
    sfCenter.SetAlignment(StringAlignmentCenter);
    sfCenter.SetLineAlignment(StringAlignmentCenter);
    sfCenter.SetFormatFlags(StringFormatFlagsNoWrap);

    StringFormat sfLeft;
    sfLeft.SetAlignment(StringAlignmentNear);
    sfLeft.SetLineAlignment(StringAlignmentCenter);
    sfLeft.SetFormatFlags(StringFormatFlagsNoWrap);

    StringFormat sfRight;
    sfRight.SetAlignment(StringAlignmentFar);
    sfRight.SetLineAlignment(StringAlignmentCenter);
    sfRight.SetFormatFlags(StringFormatFlagsNoWrap);

    LiveStatus status = m_engine.GetStatus();

    // ==========================================
    // 1. HEADER ROW: Brand & Drive Pill
    // Vertically stacked brand to prevent ANY overlap
    // ==========================================
    g.DrawString(L"RW\u2022BENCH", -1, &fontBrand, PointF(24.0f, 16.0f), &brushWhite);
    g.DrawString(L"Direct I/O Storage Benchmark", -1, &fontSubBrand, PointF(24.0f, 36.0f), &brushMuted);

    // Drive Selector Pill (x: 446, y: 16, w: 310, h: 34)
    RectF rectDrive(446.0f, 16.0f, 310.0f, 34.0f);
    bool driveHover = (m_hoverId == 1 && !status.isRunning);
    SolidBrush brushDriveBg(driveHover ? colorPillHover : colorCard);
    FillRoundedRect(g, brushDriveBg, rectDrive, 6.0f);
    DrawRoundedRect(g, penCard, rectDrive, 6.0f);

    std::wstring driveDisplay = L"No Drives Available";
    if (!m_drives.empty() && m_selectedDriveIndex >= 0 && m_selectedDriveIndex < static_cast<int>(m_drives.size())) {
        driveDisplay = m_drives[m_selectedDriveIndex].GetShortName() + L"  \u25BE";
    }
    RectF rectDriveText(458.0f, 16.0f, 286.0f, 34.0f);
    g.DrawString(driveDisplay.c_str(), -1, &fontPillBold, rectDriveText, &sfLeft, &brushWhite);

    // ==========================================
    // 2. CONTROLS ROW: Mode, Size & Run Button
    // Generous widths with NoWrap to eliminate clipping
    // ==========================================
    struct ModePillDef { const wchar_t* label; float x; float w; };
    ModePillDef modePills[4] = {
        { L"Full Suite", 24.0f,  84.0f },
        { L"Seq Write",  114.0f, 80.0f },
        { L"Seq Read",   200.0f, 80.0f },
        { L"Random 4K",  286.0f, 88.0f }
    };

    Pen penActivePill(colorActiveBlue, 1.2f);
    for (int i = 0; i < 4; ++i) {
        RectF rPill(modePills[i].x, 62.0f, modePills[i].w, 32.0f);
        bool isSel = (m_selectedMode == i);
        bool isHov = (m_hoverId == (10 + i) && !status.isRunning);

        Color bg = isSel ? Color(255, 38, 44, 56) : (isHov ? colorPillHover : colorCard);
        SolidBrush bPill(bg);
        FillRoundedRect(g, bPill, rPill, 5.0f);

        if (isSel) {
            DrawRoundedRect(g, penActivePill, rPill, 5.0f);
        } else {
            DrawRoundedRect(g, penCard, rPill, 5.0f);
        }

        Brush* pTextBrush = isSel ? &brushWhite : &brushMuted;
        Font* pFont = isSel ? &fontPillBold : &fontPill;
        g.DrawString(modePills[i].label, -1, pFont, rPill, &sfCenter, pTextBrush);
    }

    // Size Pills: [ 128 MB ] [ 256 MB ] [ 512 MB ] [ 1 GB ]
    ModePillDef sizePills[4] = {
        { L"128M", 390.0f, 52.0f },
        { L"256M", 447.0f, 52.0f },
        { L"512M", 504.0f, 52.0f },
        { L"1 GB", 561.0f, 50.0f }
    };

    for (int i = 0; i < 4; ++i) {
        RectF rPill(sizePills[i].x, 62.0f, sizePills[i].w, 32.0f);
        bool isSel = (m_selectedSize == i);
        bool isHov = (m_hoverId == (20 + i) && !status.isRunning);

        Color bg = isSel ? Color(255, 38, 44, 56) : (isHov ? colorPillHover : colorCard);
        SolidBrush bPill(bg);
        FillRoundedRect(g, bPill, rPill, 5.0f);

        if (isSel) {
            DrawRoundedRect(g, penActivePill, rPill, 5.0f);
        } else {
            DrawRoundedRect(g, penCard, rPill, 5.0f);
        }

        Brush* pTextBrush = isSel ? &brushWhite : &brushMuted;
        Font* pFont = isSel ? &fontPillBold : &fontPill;
        g.DrawString(sizePills[i].label, -1, pFont, rPill, &sfCenter, pTextBrush);
    }

    // Action Button: Start / Stop (x: 626, y: 60, w: 130, h: 34)
    RectF rectBtn(626.0f, 60.0f, 130.0f, 34.0f);
    bool btnHov = (m_hoverId == 30);

    if (status.isRunning) {
        Color cStopBg = btnHov ? Color(255, 248, 81, 73) : Color(255, 218, 54, 51);
        SolidBrush bStop(cStopBg);
        FillRoundedRect(g, bStop, rectBtn, 6.0f);
        g.DrawString(L"\u25A0  Stop", -1, &fontBtn, rectBtn, &sfCenter, &brushWhite);
    } else {
        Color cStartBg = btnHov ? Color(255, 46, 160, 67) : Color(255, 35, 134, 54);
        SolidBrush bStart(cStartBg);
        FillRoundedRect(g, bStart, rectBtn, 6.0f);
        g.DrawString(L"\u25B6  Run", -1, &fontBtn, rectBtn, &sfCenter, &brushWhite);
    }

    // ==========================================
    // 3. HERO SPEED CARD (Y: 108, H: 136, W: 732)
    // Ample vertical padding: Speed at Y:118, Status at Y:186
    // ==========================================
    RectF rectHero(24.0f, 108.0f, 732.0f, 136.0f);
    FillRoundedRect(g, brushCard, rectHero, 8.0f);
    DrawRoundedRect(g, penCard, rectHero, 8.0f);

    // Hero Live Speed Readout
    std::wostringstream ssSpeed;
    ssSpeed << std::fixed << std::setprecision(1) << status.instantMBps;
    std::wstring speedStr = ssSpeed.str();

    PointF ptSpeed(40.0f, 118.0f);
    Brush* pSpeedColor = &brushWhite;
    if (status.isRunning) {
        if (status.activeTest.find("Write") != std::string::npos) {
            pSpeedColor = &brushCyan;
        } else {
            pSpeedColor = &brushEmerald;
        }
    } else if (status.phase == "Completed") {
        pSpeedColor = &brushEmerald;
    }

    g.DrawString(speedStr.c_str(), -1, &fontSpeed, ptSpeed, pSpeedColor);

    RectF boundSpeed;
    g.MeasureString(speedStr.c_str(), -1, &fontSpeed, ptSpeed, &boundSpeed);
    PointF ptUnit(boundSpeed.X + boundSpeed.Width + 8.0f, 142.0f);
    g.DrawString(L"MB/s", -1, &fontSpeedUnit, ptUnit, &brushMuted);

    // Status label below speed (at Y: 186, giving 16px of clear air)
    std::wstring statusText = L"\u25CF Ready to benchmark";
    Brush* pStatusBrush = &brushMuted;
    if (status.isRunning) {
        statusText = L"\u25CF " + std::wstring(status.activeTest.begin(), status.activeTest.end());
        pStatusBrush = (status.activeTest.find("Write") != std::string::npos) ? &brushCyan : &brushEmerald;
    } else if (status.phase == "Completed") {
        statusText = L"\u25CF Benchmark Completed";
        pStatusBrush = &brushEmerald;
    } else if (status.phase == "Aborted") {
        statusText = L"\u25CF Benchmark Stopped";
        pStatusBrush = &brushMuted;
    }

    PointF ptStatus(40.0f, 186.0f);
    g.DrawString(statusText.c_str(), -1, &fontStatus, ptStatus, pStatusBrush);

    // Live Stats on the right side of Hero Card (Right-aligned values)
    float statY = 120.0f;
    std::wostringstream ssPeak, ssAvg, ssIOPS;
    ssPeak << std::fixed << std::setprecision(1) << status.peakMBps << L" MB/s";
    ssAvg << std::fixed << std::setprecision(1) << status.averageMBps << L" MB/s";
    ssIOPS << status.currentIOPS;

    RectF rPeakLbl(480.0f, statY, 140.0f, 20.0f);
    RectF rPeakVal(610.0f, statY, 134.0f, 20.0f);
    g.DrawString(L"Peak Speed", -1, &fontStatLabel, rPeakLbl, &sfLeft, &brushMuted);
    g.DrawString(ssPeak.str().c_str(), -1, &fontStatValue, rPeakVal, &sfRight, &brushWhite);

    RectF rAvgLbl(480.0f, statY + 24.0f, 140.0f, 20.0f);
    RectF rAvgVal(610.0f, statY + 24.0f, 134.0f, 20.0f);
    g.DrawString(L"Average Speed", -1, &fontStatLabel, rAvgLbl, &sfLeft, &brushMuted);
    g.DrawString(ssAvg.str().c_str(), -1, &fontStatValue, rAvgVal, &sfRight, &brushWhite);

    RectF rIopsLbl(480.0f, statY + 48.0f, 140.0f, 20.0f);
    RectF rIopsVal(610.0f, statY + 48.0f, 134.0f, 20.0f);
    g.DrawString(L"Current IOPS", -1, &fontStatLabel, rIopsLbl, &sfLeft, &brushMuted);
    g.DrawString(ssIOPS.str().c_str(), -1, &fontStatValue, rIopsVal, &sfRight, &brushWhite);

    // Slim Progress Track at card bottom
    float pbX = 40.0f;
    float pbY = 228.0f;
    float pbW = 692.0f;
    float pbH = 3.0f;

    SolidBrush brushPbTrack(Color(255, 33, 38, 45));
    RectF rectPbTrack(pbX, pbY, pbW, pbH);
    FillRoundedRect(g, brushPbTrack, rectPbTrack, 1.5f);

    float fillW = pbW * (status.progress / 100.0f);
    if (fillW > 2.0f) {
        SolidBrush brushPbFill(colorActiveBlue);
        RectF rectPbFill(pbX, pbY, fillW, pbH);
        FillRoundedRect(g, brushPbFill, rectPbFill, 1.5f);
    }

    // ==========================================
    // 4. THROUGHPUT GRAPH CARD (Y: 254, H: 156, W: 732)
    // ==========================================
    RectF rectGraph(24.0f, 254.0f, 732.0f, 156.0f);
    FillRoundedRect(g, brushCard, rectGraph, 8.0f);
    DrawRoundedRect(g, penCard, rectGraph, 8.0f);

    g.DrawString(L"THROUGHPUT HISTORY", -1, &fontCardTitle, PointF(40.0f, 264.0f), &brushMuted);

    float plotX = 40.0f;
    float plotY = 286.0f;
    float plotW = 676.0f;
    float plotH = 110.0f;

    // Faint baseline guides
    g.DrawLine(&penDivider, plotX, plotY + (plotH / 2.0f), plotX + plotW, plotY + (plotH / 2.0f));
    g.DrawLine(&penDivider, plotX, plotY + plotH, plotX + plotW, plotY + plotH);

    double maxSpeed = 100.0;
    for (const auto& smp : status.history) {
        if (smp.instantMBps > maxSpeed) maxSpeed = smp.instantMBps;
    }
    maxSpeed *= 1.15;

    std::wostringstream ssMaxScale;
    ssMaxScale << std::fixed << std::setprecision(0) << maxSpeed << L" MB/s";
    RectF rectMaxScale(plotX, 264.0f, plotW, 16.0f);
    g.DrawString(ssMaxScale.str().c_str(), -1, &fontSubBrand, rectMaxScale, &sfRight, &brushDim);

    if (status.history.size() >= 2) {
        std::vector<PointF> pts;
        float stepX = plotW / static_cast<float>(status.history.size() - 1);

        for (size_t i = 0; i < status.history.size(); ++i) {
            float px = plotX + static_cast<float>(i) * stepX;
            float py = plotY + plotH - static_cast<float>((status.history[i].instantMBps / maxSpeed) * plotH);
            if (py < plotY) py = plotY;
            if (py > plotY + plotH) py = plotY + plotH;
            pts.push_back(PointF(px, py));
        }

        // Soft semi-transparent gradient fill under line
        GraphicsPath fillPath;
        fillPath.AddLine(PointF(plotX, plotY + plotH), pts[0]);
        for (size_t i = 1; i < pts.size(); ++i) {
            fillPath.AddLine(pts[i - 1], pts[i]);
        }
        fillPath.AddLine(pts.back(), PointF(pts.back().X, plotY + plotH));
        fillPath.CloseFigure();

        SolidBrush brushArea(Color(35, 88, 166, 255));
        g.FillPath(&brushArea, &fillPath);

        // Smooth curve
        Pen penCurve(colorActiveBlue, 1.8f);
        for (size_t i = 1; i < pts.size(); ++i) {
            g.DrawLine(&penCurve, pts[i - 1], pts[i]);
        }

        // Glowing pulse head point
        PointF head = pts.back();
        SolidBrush brushHead(colorActiveBlue);
        g.FillEllipse(&brushHead, head.X - 3.5f, head.Y - 3.5f, 7.0f, 7.0f);
    }

    // ==========================================
    // 5. RESULTS SUMMARY CARD (Y: 420, H: 140, W: 732)
    // Generous vertical spacing: Header Y:432, Value Y:456, Sub Y:510
    // ==========================================
    RectF rectResults(24.0f, 420.0f, 732.0f, 140.0f);
    FillRoundedRect(g, brushCard, rectResults, 8.0f);
    DrawRoundedRect(g, penCard, rectResults, 8.0f);

    float colW = 732.0f / 4.0f;

    auto findResult = [&](const std::string& key1, const std::string& key2) -> const BenchmarkResult* {
        for (const auto& r : status.completedResults) {
            if (r.testName.find(key1) != std::string::npos && r.testName.find(key2) != std::string::npos) {
                return &r;
            }
        }
        return nullptr;
    };

    struct ColData {
        const wchar_t* label;
        const BenchmarkResult* res;
        bool isWriting;
        bool isCurrentlyRunning;
    };

    ColData cols[4] = {
        { L"SEQ WRITE (1M)", findResult("Sequential", "Write"), true,
          status.isRunning && status.activeTest.find("Sequential Write") != std::string::npos },
        { L"SEQ READ (1M)",  findResult("Sequential", "Read"), false,
          status.isRunning && status.activeTest.find("Sequential Read") != std::string::npos },
        { L"RND 4K WRITE",   findResult("Random", "Write"), true,
          status.isRunning && status.activeTest.find("Random 4K Write") != std::string::npos },
        { L"RND 4K READ",    findResult("Random", "Read"), false,
          status.isRunning && status.activeTest.find("Random 4K Read") != std::string::npos }
    };

    for (int i = 0; i < 4; ++i) {
        float cx = 24.0f + colW * i;

        if (i > 0) {
            g.DrawLine(&penDivider, cx, 432.0f, cx, 546.0f);
        }

        // Header Label
        g.DrawString(cols[i].label, -1, &fontColLabel, PointF(cx + 18.0f, 432.0f), &brushMuted);

        // Main Speed Value
        if (cols[i].res) {
            std::wostringstream ssVal;
            ssVal << std::fixed << std::setprecision(1) << cols[i].res->avgMBps << L" MB/s";
            Brush* pColor = cols[i].isWriting ? &brushCyan : &brushEmerald;
            g.DrawString(ssVal.str().c_str(), -1, &fontColValue, PointF(cx + 18.0f, 456.0f), pColor);

            std::wostringstream ssSub;
            if (i < 2) {
                ssSub << L"Peak: " << std::fixed << std::setprecision(0) << cols[i].res->peakMBps << L" MB/s";
            } else {
                ssSub << cols[i].res->avgIOPS << L" IOPS";
            }
            g.DrawString(ssSub.str().c_str(), -1, &fontColSub, PointF(cx + 18.0f, 510.0f), &brushMuted);
        } else if (cols[i].isCurrentlyRunning) {
            std::wostringstream ssVal;
            ssVal << std::fixed << std::setprecision(1) << status.instantMBps << L" MB/s";
            g.DrawString(ssVal.str().c_str(), -1, &fontColValue, PointF(cx + 18.0f, 456.0f), &brushCyan);
            g.DrawString(L"Running...", -1, &fontColSub, PointF(cx + 18.0f, 510.0f), &brushEmerald);
        } else {
            g.DrawString(L"-- MB/s", -1, &fontColValue, PointF(cx + 18.0f, 456.0f), &brushDim);
            g.DrawString(L"Pending", -1, &fontColSub, PointF(cx + 18.0f, 510.0f), &brushDim);
        }
    }

    // Blit to Screen (Double Buffer Complete)
    HDC hdcScreen = GetDC(m_hWnd);
    BitBlt(hdcScreen, 0, 0, width, height, m_hMemDC, 0, 0, SRCCOPY);
    ReleaseDC(m_hWnd, hdcScreen);
}
