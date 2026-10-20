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
    wc.lpszClassName = L"RWBenchWindowClass";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.style = CS_HREDRAW | CS_VREDRAW;

    RegisterClassExW(&wc);

    // Compact, balanced window dimensions: 640 x 450 px
    int clientW = 640;
    int clientH = 450;

    RECT rc = { 0, 0, clientW, clientH };
    AdjustWindowRectEx(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE, 0);

    int winW = rc.right - rc.left;
    int winH = rc.bottom - rc.top;
    int posX = (GetSystemMetrics(SM_CXSCREEN) - winW) / 2;
    int posY = (GetSystemMetrics(SM_CYSCREEN) - winH) / 2;

    m_hWnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"RW-Bench",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY, winW, winH,
        nullptr, nullptr, wc.hInstance, this
    );

    if (!m_hWnd) return false;

    // Windows 10/11 Immersive Dark Titlebar
    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(m_hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));

    PopulateDrives();

    // 30 FPS update timer
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
    if (y >= 14 && y <= 44) {
        if (x >= 236 && x <= 320) return 1; // Mode pill
        if (x >= 326 && x <= 400) return 2; // Size pill
        if (x >= 406 && x <= 546) return 3; // Drive pill
        if (x >= 552 && x <= 624) return 4; // Start/Stop button
    }
    return 0;
}

void MainWindow::ShowModeMenu() {
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    AppendMenuW(hMenu, MF_STRING | (m_selectedMode == 0 ? MF_CHECKED : 0), 2000, L"Full Suite (Seq + 4K)");
    AppendMenuW(hMenu, MF_STRING | (m_selectedMode == 1 ? MF_CHECKED : 0), 2001, L"Sequential Write Only");
    AppendMenuW(hMenu, MF_STRING | (m_selectedMode == 2 ? MF_CHECKED : 0), 2002, L"Sequential Read Only");
    AppendMenuW(hMenu, MF_STRING | (m_selectedMode == 3 ? MF_CHECKED : 0), 2003, L"Random 4K Only");

    POINT pt = { 236, 46 };
    ClientToScreen(m_hWnd, &pt);

    int cmd = TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x, pt.y, m_hWnd, nullptr);
    DestroyMenu(hMenu);

    if (cmd >= 2000 && cmd <= 2003) {
        m_selectedMode = cmd - 2000;
        InvalidateRect(m_hWnd, nullptr, FALSE);
    }
}

void MainWindow::ShowSizeMenu() {
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    AppendMenuW(hMenu, MF_STRING | (m_selectedSize == 0 ? MF_CHECKED : 0), 2100, L"128 MB");
    AppendMenuW(hMenu, MF_STRING | (m_selectedSize == 1 ? MF_CHECKED : 0), 2101, L"256 MB");
    AppendMenuW(hMenu, MF_STRING | (m_selectedSize == 2 ? MF_CHECKED : 0), 2102, L"512 MB");
    AppendMenuW(hMenu, MF_STRING | (m_selectedSize == 3 ? MF_CHECKED : 0), 2103, L"1 GB");

    POINT pt = { 326, 46 };
    ClientToScreen(m_hWnd, &pt);

    int cmd = TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, pt.x, pt.y, m_hWnd, nullptr);
    DestroyMenu(hMenu);

    if (cmd >= 2100 && cmd <= 2103) {
        m_selectedSize = cmd - 2100;
        InvalidateRect(m_hWnd, nullptr, FALSE);
    }
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

    POINT pt = { 406, 46 };
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

    // Reset saved histories on start
    m_writeHistory.clear();
    m_readHistory.clear();

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
                if (!s.isRunning) ShowModeMenu();
            } else if (hit == 2) {
                if (!s.isRunning) ShowSizeMenu();
            } else if (hit == 3) {
                if (!s.isRunning) ShowDriveMenu();
            } else if (hit == 4) {
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

    // Cohesive Theme Colors
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

    // Segoe UI Typefaces
    Font fontTitle(L"Segoe UI", 12.0f, FontStyleBold, UnitPoint);
    Font fontSubTitle(L"Segoe UI", 8.5f, FontStyleRegular, UnitPoint);
    Font fontPill(L"Segoe UI", 8.5f, FontStyleBold, UnitPoint);
    Font fontCardTag(L"Segoe UI", 10.0f, FontStyleBold, UnitPoint);
    Font fontCardSub(L"Segoe UI", 7.5f, FontStyleBold, UnitPoint);
    Font fontHeroSpeed(L"Segoe UI", 32.0f, FontStyleBold, UnitPoint);
    Font fontHeroUnit(L"Segoe UI", 11.5f, FontStyleBold, UnitPoint);
    Font fontHeroStat(L"Segoe UI", 8.0f, FontStyleRegular, UnitPoint);
    Font fontThHeader(L"Segoe UI", 8.0f, FontStyleBold, UnitPoint);
    Font fontRowTitle(L"Segoe UI", 9.5f, FontStyleBold, UnitPoint);
    Font fontRowSub(L"Segoe UI", 7.5f, FontStyleRegular, UnitPoint);
    Font fontRowVal(L"Segoe UI", 13.5f, FontStyleBold, UnitPoint);
    Font fontRowLat(L"Segoe UI", 8.5f, FontStyleRegular, UnitPoint);
    Font fontBtn(L"Segoe UI", 9.5f, FontStyleBold, UnitPoint);
    Font fontFooter(L"Segoe UI", 8.5f, FontStyleBold, UnitPoint);

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

    // Cache live history to respective cards
    if (status.isRunning) {
        if (status.activeTest.find("Write") != std::string::npos) {
            m_writeHistory = status.history;
        } else if (status.activeTest.find("Read") != std::string::npos) {
            m_readHistory = status.history;
        }
    }

    // ========================================================
    // 1. TOP CONTROL BAR (Y: 14 to 44, Height 30px)
    // ========================================================
    g.DrawString(L"RW-Bench", -1, &fontTitle, PointF(16.0f, 18.0f), &brushWhite);
    g.DrawString(L"Disk Speed Benchmark", -1, &fontSubTitle, PointF(96.0f, 21.0f), &brushMuted);

    // 1.1 Mode Pill (x: 236..320, w: 84)
    RectF rMode(236.0f, 14.0f, 84.0f, 30.0f);
    bool modeHov = (m_hoverId == 1 && !status.isRunning);
    SolidBrush bMode(modeHov ? colorPillHover : colorCard);
    FillRoundedRect(g, bMode, rMode, 5.0f);
    DrawRoundedRect(g, penCard, rMode, 5.0f);

    const wchar_t* modeNames[4] = { L"Full Suite \u25BE", L"Write Only \u25BE", L"Read Only \u25BE", L"4K Only \u25BE" };
    g.DrawString(modeNames[m_selectedMode], -1, &fontPill, rMode, &sfCenter, &brushWhite);

    // 1.2 Size Pill (x: 326..400, w: 74)
    RectF rSize(326.0f, 14.0f, 74.0f, 30.0f);
    bool sizeHov = (m_hoverId == 2 && !status.isRunning);
    SolidBrush bSize(sizeHov ? colorPillHover : colorCard);
    FillRoundedRect(g, bSize, rSize, 5.0f);
    DrawRoundedRect(g, penCard, rSize, 5.0f);

    const wchar_t* sizeNames[4] = { L"128 MB \u25BE", L"256 MB \u25BE", L"512 MB \u25BE", L"1 GB \u25BE" };
    g.DrawString(sizeNames[m_selectedSize], -1, &fontPill, rSize, &sfCenter, &brushWhite);

    // 1.3 Drive Pill (x: 406..546, w: 140)
    RectF rDrive(406.0f, 14.0f, 140.0f, 30.0f);
    bool driveHov = (m_hoverId == 3 && !status.isRunning);
    SolidBrush bDrive(driveHov ? colorPillHover : colorCard);
    FillRoundedRect(g, bDrive, rDrive, 5.0f);
    DrawRoundedRect(g, penCard, rDrive, 5.0f);

    std::wstring driveShort = L"No Drives \u25BE";
    if (!m_drives.empty() && m_selectedDriveIndex >= 0 && m_selectedDriveIndex < static_cast<int>(m_drives.size())) {
        driveShort = m_drives[m_selectedDriveIndex].GetVeryShortName() + L" \u25BE";
    }
    RectF rDriveText(414.0f, 14.0f, 126.0f, 30.0f);
    g.DrawString(driveShort.c_str(), -1, &fontPill, rDriveText, &sfLeft, &brushWhite);

    // 1.4 Start / Stop Action Button (x: 552..624, w: 72)
    RectF rBtn(552.0f, 14.0f, 72.0f, 30.0f);
    bool btnHov = (m_hoverId == 4);

    if (status.isRunning) {
        Color cStop = btnHov ? Color(255, 248, 81, 73) : Color(255, 218, 54, 51);
        SolidBrush bStop(cStop);
        FillRoundedRect(g, bStop, rBtn, 5.0f);
        g.DrawString(L"\u25A0 Stop", -1, &fontBtn, rBtn, &sfCenter, &brushWhite);
    } else {
        Color cStart = btnHov ? Color(255, 46, 160, 67) : Color(255, 35, 134, 54);
        SolidBrush bStart(cStart);
        FillRoundedRect(g, bStart, rBtn, 5.0f);
        g.DrawString(L"\u25B6 Run", -1, &fontBtn, rBtn, &sfCenter, &brushWhite);
    }

    // Helper to find completed results
    auto findResult = [&](const std::string& key1, const std::string& key2) -> const BenchmarkResult* {
        for (const auto& r : status.completedResults) {
            if (r.testName.find(key1) != std::string::npos && r.testName.find(key2) != std::string::npos) {
                return &r;
            }
        }
        return nullptr;
    };

    const BenchmarkResult* resSeqW = findResult("Sequential", "Write");
    const BenchmarkResult* resSeqR = findResult("Sequential", "Read");
    const BenchmarkResult* res4KW  = findResult("Random", "Write");
    const BenchmarkResult* res4KR  = findResult("Random", "Read");

    bool isWriting = status.isRunning && (status.activeTest.find("Write") != std::string::npos);
    bool isReading = status.isRunning && (status.activeTest.find("Read") != std::string::npos);

    // ========================================================
    // 2. DUAL HERO CARDS (Y: 52 to 216, Height 164px)
    // Left: WRITE SPEED | Right: READ SPEED
    // ========================================================
    float cardW = 298.0f;
    float cardH = 164.0f;
    float cardY = 52.0f;

    // --------------------------------------------------------
    // 2.1 LEFT CARD: WRITE SPEED
    // --------------------------------------------------------
    RectF rWriteCard(16.0f, cardY, cardW, cardH);
    FillRoundedRect(g, brushCard, rWriteCard, 7.0f);
    DrawRoundedRect(g, penCard, rWriteCard, 7.0f);

    // Header Tag
    g.DrawString(L"WRITE", -1, &fontCardTag, PointF(28.0f, 62.0f), &brushCyan);

    std::wstring writeTag = L"DIRECT I/O";
    Brush* pWriteTagBrush = &brushMuted;
    if (isWriting) {
        writeTag = L"\u25CF ACTIVE";
        pWriteTagBrush = &brushCyan;
    } else if (resSeqW) {
        writeTag = L"\u25CF FINISHED";
        pWriteTagBrush = &brushMuted;
    }
    RectF rWTagRight(16.0f, 64.0f, cardW - 24.0f, 16.0f);
    g.DrawString(writeTag.c_str(), -1, &fontCardSub, rWTagRight, &sfRight, pWriteTagBrush);

    // Speed Value
    std::wstring wSpeedNum = L"--";
    double wPeak = 0.0, wAvg = 0.0;
    uint32_t wIops = 0;

    if (isWriting) {
        std::wostringstream ss;
        ss << std::fixed << std::setprecision(1) << status.instantMBps;
        wSpeedNum = ss.str();
        wPeak = status.peakMBps;
        wAvg = status.averageMBps;
        wIops = status.currentIOPS;
    } else if (resSeqW) {
        std::wostringstream ss;
        ss << std::fixed << std::setprecision(1) << resSeqW->avgMBps;
        wSpeedNum = ss.str();
        wPeak = resSeqW->peakMBps;
        wAvg = resSeqW->avgMBps;
        wIops = resSeqW->avgIOPS;
    }

    PointF ptWSpeed(28.0f, 84.0f);
    Brush* pWNumBrush = isWriting ? &brushCyan : (wSpeedNum == L"--" ? &brushDim : &brushWhite);
    g.DrawString(wSpeedNum.c_str(), -1, &fontHeroSpeed, ptWSpeed, pWNumBrush);

    RectF boundWSpeed;
    g.MeasureString(wSpeedNum.c_str(), -1, &fontHeroSpeed, ptWSpeed, &boundWSpeed);
    PointF ptWUnit(boundWSpeed.X + boundWSpeed.Width + 6.0f, 102.0f);
    g.DrawString(L"MB/s", -1, &fontHeroUnit, ptWUnit, &brushMuted);

    // Write Sub-metric
    std::wostringstream ssWSub;
    if (wPeak > 0.0) {
        ssWSub << L"Peak: " << std::fixed << std::setprecision(0) << wPeak << L" MB/s   Avg: " << wAvg << L" MB/s";
    } else {
        ssWSub << L"Direct physical write throughput";
    }
    g.DrawString(ssWSub.str().c_str(), -1, &fontHeroStat, PointF(28.0f, 134.0f), &brushMuted);

    // Mini Sparkline inside Write card (Y: 154 to 206, H: 52px)
    float spWX = 28.0f;
    float spWY = 154.0f;
    float spWW = cardW - 24.0f;
    float spWH = 50.0f;

    g.DrawLine(&penDivider, spWX, spWY + spWH, spWX + spWW, spWY + spWH);

    const auto& wHist = isWriting ? status.history : m_writeHistory;
    if (wHist.size() >= 2) {
        double maxW = 50.0;
        for (const auto& smp : wHist) if (smp.instantMBps > maxW) maxW = smp.instantMBps;
        maxW *= 1.15;

        std::vector<PointF> pts;
        float stepX = spWW / static_cast<float>(wHist.size() - 1);
        for (size_t i = 0; i < wHist.size(); ++i) {
            float px = spWX + static_cast<float>(i) * stepX;
            float py = spWY + spWH - static_cast<float>((wHist[i].instantMBps / maxW) * spWH);
            if (py < spWY) py = spWY;
            if (py > spWY + spWH) py = spWY + spWH;
            pts.push_back(PointF(px, py));
        }

        GraphicsPath fillPath;
        fillPath.AddLine(PointF(spWX, spWY + spWH), pts[0]);
        for (size_t i = 1; i < pts.size(); ++i) fillPath.AddLine(pts[i - 1], pts[i]);
        fillPath.AddLine(pts.back(), PointF(pts.back().X, spWY + spWH));
        fillPath.CloseFigure();

        SolidBrush areaBrush(Color(30, 88, 166, 255));
        g.FillPath(&areaBrush, &fillPath);

        Pen penCurve(colorActiveBlue, 1.5f);
        for (size_t i = 1; i < pts.size(); ++i) g.DrawLine(&penCurve, pts[i - 1], pts[i]);

        if (isWriting) {
            PointF head = pts.back();
            SolidBrush bHead(colorActiveBlue);
            g.FillEllipse(&bHead, head.X - 3.0f, head.Y - 3.0f, 6.0f, 6.0f);
        }
    }

    // --------------------------------------------------------
    // 2.2 RIGHT CARD: READ SPEED
    // --------------------------------------------------------
    RectF rReadCard(326.0f, cardY, cardW, cardH);
    FillRoundedRect(g, brushCard, rReadCard, 7.0f);
    DrawRoundedRect(g, penCard, rReadCard, 7.0f);

    // Header Tag
    g.DrawString(L"READ", -1, &fontCardTag, PointF(338.0f, 62.0f), &brushEmerald);

    std::wstring readTag = L"DIRECT I/O";
    Brush* pReadTagBrush = &brushMuted;
    if (isReading) {
        readTag = L"\u25CF ACTIVE";
        pReadTagBrush = &brushEmerald;
    } else if (resSeqR) {
        readTag = L"\u25CF FINISHED";
        pReadTagBrush = &brushMuted;
    }
    RectF rRTagRight(326.0f, 64.0f, cardW - 24.0f, 16.0f);
    g.DrawString(readTag.c_str(), -1, &fontCardSub, rRTagRight, &sfRight, pReadTagBrush);

    // Read Speed Value
    std::wstring rSpeedNum = L"--";
    double rPeak = 0.0, rAvg = 0.0;
    uint32_t rIops = 0;

    if (isReading) {
        std::wostringstream ss;
        ss << std::fixed << std::setprecision(1) << status.instantMBps;
        rSpeedNum = ss.str();
        rPeak = status.peakMBps;
        rAvg = status.averageMBps;
        rIops = status.currentIOPS;
    } else if (resSeqR) {
        std::wostringstream ss;
        ss << std::fixed << std::setprecision(1) << resSeqR->avgMBps;
        rSpeedNum = ss.str();
        rPeak = resSeqR->peakMBps;
        rAvg = resSeqR->avgMBps;
        rIops = resSeqR->avgIOPS;
    }

    PointF ptRSpeed(338.0f, 84.0f);
    Brush* pRNumBrush = isReading ? &brushEmerald : (rSpeedNum == L"--" ? &brushDim : &brushWhite);
    g.DrawString(rSpeedNum.c_str(), -1, &fontHeroSpeed, ptRSpeed, pRNumBrush);

    RectF boundRSpeed;
    g.MeasureString(rSpeedNum.c_str(), -1, &fontHeroSpeed, ptRSpeed, &boundRSpeed);
    PointF ptRUnit(boundRSpeed.X + boundRSpeed.Width + 6.0f, 102.0f);
    g.DrawString(L"MB/s", -1, &fontHeroUnit, ptRUnit, &brushMuted);

    // Read Sub-metric
    std::wostringstream ssRSub;
    if (rPeak > 0.0) {
        ssRSub << L"Peak: " << std::fixed << std::setprecision(0) << rPeak << L" MB/s   Avg: " << rAvg << L" MB/s";
    } else {
        ssRSub << L"Direct unbuffered read throughput";
    }
    g.DrawString(ssRSub.str().c_str(), -1, &fontHeroStat, PointF(338.0f, 134.0f), &brushMuted);

    // Mini Sparkline inside Read card (Y: 154 to 206, H: 52px)
    float spRX = 338.0f;
    float spRY = 154.0f;
    float spRW = cardW - 24.0f;
    float spRH = 50.0f;

    g.DrawLine(&penDivider, spRX, spRY + spRH, spRX + spRW, spRY + spRH);

    const auto& rHist = isReading ? status.history : m_readHistory;
    if (rHist.size() >= 2) {
        double maxR = 50.0;
        for (const auto& smp : rHist) if (smp.instantMBps > maxR) maxR = smp.instantMBps;
        maxR *= 1.15;

        std::vector<PointF> pts;
        float stepX = spRW / static_cast<float>(rHist.size() - 1);
        for (size_t i = 0; i < rHist.size(); ++i) {
            float px = spRX + static_cast<float>(i) * stepX;
            float py = spRY + spRH - static_cast<float>((rHist[i].instantMBps / maxR) * spRH);
            if (py < spRY) py = spRY;
            if (py > spRY + spRH) py = spRY + spRH;
            pts.push_back(PointF(px, py));
        }

        GraphicsPath fillPath;
        fillPath.AddLine(PointF(spRX, spRY + spRH), pts[0]);
        for (size_t i = 1; i < pts.size(); ++i) fillPath.AddLine(pts[i - 1], pts[i]);
        fillPath.AddLine(pts.back(), PointF(pts.back().X, spRY + spRH));
        fillPath.CloseFigure();

        SolidBrush areaBrush(Color(30, 63, 185, 80));
        g.FillPath(&areaBrush, &fillPath);

        Pen penCurve(colorEmerald, 1.5f);
        for (size_t i = 1; i < pts.size(); ++i) g.DrawLine(&penCurve, pts[i - 1], pts[i]);

        if (isReading) {
            PointF head = pts.back();
            SolidBrush bHead(colorEmerald);
            g.FillEllipse(&bHead, head.X - 3.0f, head.Y - 3.0f, 6.0f, 6.0f);
        }
    }

    // ========================================================
    // 3. DETAILED RESULTS BREAKDOWN TABLE CARD (Y: 226 to 434, Height 208px)
    // ========================================================
    RectF rTable(16.0f, 226.0f, 608.0f, 208.0f);
    FillRoundedRect(g, brushCard, rTable, 7.0f);
    DrawRoundedRect(g, penCard, rTable, 7.0f);

    // 3.1 Table Header Row (Y = 236)
    g.DrawString(L"TEST PROFILE", -1, &fontThHeader, PointF(30.0f, 236.0f), &brushMuted);
    g.DrawString(L"WRITE SPEED", -1, &fontThHeader, PointF(230.0f, 236.0f), &brushCyan);
    g.DrawString(L"READ SPEED",  -1, &fontThHeader, PointF(370.0f, 236.0f), &brushEmerald);
    g.DrawString(L"LATENCY / IOPS", -1, &fontThHeader, PointF(500.0f, 236.0f), &brushMuted);

    g.DrawLine(&penDivider, 24.0f, 254.0f, 616.0f, 254.0f);

    // 3.2 Row 1: Sequential 1MB (Y: 260 to 304)
    g.DrawString(L"Sequential (1 MB)", -1, &fontRowTitle, PointF(30.0f, 260.0f), &brushWhite);
    g.DrawString(L"Continuous block stream", -1, &fontRowSub, PointF(30.0f, 278.0f), &brushMuted);

    // Seq Write cell
    if (resSeqW) {
        std::wostringstream ss; ss << std::fixed << std::setprecision(1) << resSeqW->avgMBps << L" MB/s";
        g.DrawString(ss.str().c_str(), -1, &fontRowVal, PointF(230.0f, 264.0f), &brushCyan);
    } else if (isWriting && status.activeTest.find("Sequential") != std::string::npos) {
        std::wostringstream ss; ss << std::fixed << std::setprecision(1) << status.instantMBps << L" MB/s";
        g.DrawString(ss.str().c_str(), -1, &fontRowVal, PointF(230.0f, 264.0f), &brushCyan);
    } else {
        g.DrawString(L"-- MB/s", -1, &fontRowVal, PointF(230.0f, 264.0f), &brushDim);
    }

    // Seq Read cell
    if (resSeqR) {
        std::wostringstream ss; ss << std::fixed << std::setprecision(1) << resSeqR->avgMBps << L" MB/s";
        g.DrawString(ss.str().c_str(), -1, &fontRowVal, PointF(370.0f, 264.0f), &brushEmerald);
    } else if (isReading && status.activeTest.find("Sequential") != std::string::npos) {
        std::wostringstream ss; ss << std::fixed << std::setprecision(1) << status.instantMBps << L" MB/s";
        g.DrawString(ss.str().c_str(), -1, &fontRowVal, PointF(370.0f, 264.0f), &brushEmerald);
    } else {
        g.DrawString(L"-- MB/s", -1, &fontRowVal, PointF(370.0f, 264.0f), &brushDim);
    }

    // Seq Latency / IOPS
    if (resSeqW || resSeqR) {
        double lat = resSeqR ? resSeqR->avgLatencyMs : resSeqW->avgLatencyMs;
        std::wostringstream ss; ss << std::fixed << std::setprecision(2) << lat << L" ms";
        g.DrawString(ss.str().c_str(), -1, &fontRowLat, PointF(500.0f, 268.0f), &brushWhite);
    } else {
        g.DrawString(L"--", -1, &fontRowLat, PointF(500.0f, 268.0f), &brushDim);
    }

    g.DrawLine(&penDivider, 24.0f, 308.0f, 616.0f, 308.0f);

    // 3.3 Row 2: Random 4K (Y: 314 to 358)
    g.DrawString(L"Random 4K", -1, &fontRowTitle, PointF(30.0f, 314.0f), &brushWhite);
    g.DrawString(L"4 KiB sector aligned IOPS", -1, &fontRowSub, PointF(30.0f, 332.0f), &brushMuted);

    // 4K Write cell
    if (res4KW) {
        std::wostringstream ss; ss << std::fixed << std::setprecision(1) << res4KW->avgMBps << L" MB/s";
        g.DrawString(ss.str().c_str(), -1, &fontRowVal, PointF(230.0f, 318.0f), &brushCyan);
    } else if (isWriting && status.activeTest.find("Random") != std::string::npos) {
        std::wostringstream ss; ss << std::fixed << std::setprecision(1) << status.instantMBps << L" MB/s";
        g.DrawString(ss.str().c_str(), -1, &fontRowVal, PointF(230.0f, 318.0f), &brushCyan);
    } else {
        g.DrawString(L"-- MB/s", -1, &fontRowVal, PointF(230.0f, 318.0f), &brushDim);
    }

    // 4K Read cell
    if (res4KR) {
        std::wostringstream ss; ss << std::fixed << std::setprecision(1) << res4KR->avgMBps << L" MB/s";
        g.DrawString(ss.str().c_str(), -1, &fontRowVal, PointF(370.0f, 318.0f), &brushEmerald);
    } else if (isReading && status.activeTest.find("Random") != std::string::npos) {
        std::wostringstream ss; ss << std::fixed << std::setprecision(1) << status.instantMBps << L" MB/s";
        g.DrawString(ss.str().c_str(), -1, &fontRowVal, PointF(370.0f, 318.0f), &brushEmerald);
    } else {
        g.DrawString(L"-- MB/s", -1, &fontRowVal, PointF(370.0f, 318.0f), &brushDim);
    }

    // 4K IOPS cell
    if (res4KR || res4KW) {
        uint32_t iops = res4KR ? res4KR->avgIOPS : res4KW->avgIOPS;
        std::wostringstream ss; ss << iops << L" IOPS";
        g.DrawString(ss.str().c_str(), -1, &fontRowLat, PointF(500.0f, 322.0f), &brushWhite);
    } else {
        g.DrawString(L"--", -1, &fontRowLat, PointF(500.0f, 322.0f), &brushDim);
    }

    g.DrawLine(&penDivider, 24.0f, 362.0f, 616.0f, 362.0f);

    // 3.4 Footer Status & Slim Progress Bar (Y: 374 to 420)
    std::wstring footStatus = L"\u25CF Ready to benchmark";
    Brush* pFootBrush = &brushMuted;
    if (status.isRunning) {
        footStatus = L"\u25CF Running " + std::wstring(status.activeTest.begin(), status.activeTest.end());
        pFootBrush = isWriting ? &brushCyan : &brushEmerald;
    } else if (status.phase == "Completed") {
        footStatus = L"\u25CF Benchmark Completed";
        pFootBrush = &brushEmerald;
    } else if (status.phase == "Aborted") {
        footStatus = L"\u25CF Benchmark Stopped";
        pFootBrush = &brushMuted;
    }
    g.DrawString(footStatus.c_str(), -1, &fontFooter, PointF(30.0f, 376.0f), pFootBrush);

    // Slim Progress Track
    float pbX = 30.0f;
    float pbY = 404.0f;
    float pbW = 580.0f;
    float pbH = 3.0f;

    SolidBrush brushPbTrack(Color(255, 33, 38, 45));
    RectF rectPbTrack(pbX, pbY, pbW, pbH);
    FillRoundedRect(g, brushPbTrack, rectPbTrack, 1.5f);

    float fillW = pbW * (status.progress / 100.0f);
    if (fillW > 2.0f) {
        SolidBrush brushPbFill(isWriting ? colorActiveBlue : colorEmerald);
        RectF rectPbFill(pbX, pbY, fillW, pbH);
        FillRoundedRect(g, brushPbFill, rectPbFill, 1.5f);
    }

    // Blit Double Buffer to Screen
    HDC hdcScreen = GetDC(m_hWnd);
    BitBlt(hdcScreen, 0, 0, width, height, m_hMemDC, 0, 0, SRCCOPY);
    ReleaseDC(m_hWnd, hdcScreen);
}
