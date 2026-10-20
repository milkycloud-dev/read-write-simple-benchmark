#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <dwmapi.h>
#include <shlobj.h>
#include <commctrl.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include "ui.hpp"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "comctl32.lib")

using namespace Gdiplus;

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

enum ControlIds {
    ID_CB_DRIVES = 201,
    ID_CB_SIZE   = 202,
    ID_CB_MODE   = 203,
    ID_BTN_START = 204
};

static void DrawRoundedRect(Graphics& g, Pen& pen, RectF rect, float radius) {
    GraphicsPath path;
    path.AddArc(rect.X, rect.Y, radius * 2, radius * 2, 180, 90);
    path.AddArc(rect.Right() - radius * 2, rect.Y, radius * 2, radius * 2, 270, 90);
    path.AddArc(rect.Right() - radius * 2, rect.Bottom() - radius * 2, radius * 2, radius * 2, 0, 90);
    path.AddArc(rect.X, rect.Bottom() - radius * 2, radius * 2, radius * 2, 90, 90);
    path.CloseFigure();
    g.DrawPath(&pen, &path);
}

static void FillRoundedRect(Graphics& g, Brush& brush, RectF rect, float radius) {
    GraphicsPath path;
    path.AddArc(rect.X, rect.Y, radius * 2, radius * 2, 180, 90);
    path.AddArc(rect.Right() - radius * 2, rect.Y, radius * 2, radius * 2, 270, 90);
    path.AddArc(rect.Right() - radius * 2, rect.Bottom() - radius * 2, radius * 2, 0, 90);
    path.AddArc(rect.X, rect.Bottom() - radius * 2, radius * 2, radius * 2, 90, 90);
    path.CloseFigure();
    g.FillPath(&brush, &path);
}

MainWindow::MainWindow(BenchmarkEngine& engine)
    : m_engine(engine) {}

MainWindow::~MainWindow() {
    CleanupMemBuffer();
    if (m_hFontUI) DeleteObject(m_hFontUI);
    if (m_hFontTitle) DeleteObject(m_hFontTitle);
    if (m_hFontSpeed) DeleteObject(m_hFontSpeed);
    if (m_hFontSmall) DeleteObject(m_hFontSmall);
}

bool MainWindow::Create() {
    WNDCLASSEXW wc = {sizeof(wc)};
    wc.lpfnWndProc = MainWindow::WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"RWBenchMinimalNativeClass";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(13, 17, 23)); // #0D1117 Minimal Dark
    wc.style = CS_HREDRAW | CS_VREDRAW;

    RegisterClassExW(&wc);

    int winW = 780;
    int winH = 540;
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

    // Windows 10/11 Immersive Dark Titlebar
    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(m_hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));

    return true;
}

void MainWindow::Show(int nCmdShow) {
    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);
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
        case WM_CREATE:
            OnCreate();
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(m_hWnd, &ps);
            OnPaint();
            EndPaint(m_hWnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1; // Handled in double buffer

        case WM_TIMER:
            OnTimer();
            return 0;

        case WM_COMMAND:
            OnCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetTextColor(hdc, RGB(230, 237, 243));
            SetBkColor(hdc, RGB(22, 27, 34));
            static HBRUSH hBr = CreateSolidBrush(RGB(22, 27, 34));
            return reinterpret_cast<LRESULT>(hBr);
        }

        case WM_DESTROY:
            OnDestroy();
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(m_hWnd, msg, wParam, lParam);
}

void MainWindow::OnCreate() {
    // Native Segoe UI Fonts
    m_hFontUI    = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hFontTitle = CreateFontW(15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hFontSpeed = CreateFontW(38, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hFontSmall = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    HINSTANCE hInst = GetModuleHandleW(nullptr);

    // 1. Drive Selection ComboBox
    m_cbDrives = CreateWindowExW(
        0, WC_COMBOBOXW, L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
        24, 20, 320, 240,
        m_hWnd, reinterpret_cast<HMENU>(ID_CB_DRIVES), hInst, nullptr
    );
    SendMessageW(m_cbDrives, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontUI), TRUE);

    // 2. Routine Mode ComboBox
    m_cbMode = CreateWindowExW(
        0, WC_COMBOBOXW, L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
        356, 20, 160, 200,
        m_hWnd, reinterpret_cast<HMENU>(ID_CB_MODE), hInst, nullptr
    );
    SendMessageW(m_cbMode, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontUI), TRUE);
    SendMessageW(m_cbMode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Full Suite"));
    SendMessageW(m_cbMode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Seq Write Only"));
    SendMessageW(m_cbMode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Seq Read Only"));
    SendMessageW(m_cbMode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Random 4K Only"));
    SendMessageW(m_cbMode, CB_SETCURSEL, 0, 0);

    // 3. Size ComboBox
    m_cbSize = CreateWindowExW(
        0, WC_COMBOBOXW, L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
        528, 20, 95, 200,
        m_hWnd, reinterpret_cast<HMENU>(ID_CB_SIZE), hInst, nullptr
    );
    SendMessageW(m_cbSize, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontUI), TRUE);
    SendMessageW(m_cbSize, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"128 MB"));
    SendMessageW(m_cbSize, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"256 MB"));
    SendMessageW(m_cbSize, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"512 MB"));
    SendMessageW(m_cbSize, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1 GB"));
    SendMessageW(m_cbSize, CB_SETCURSEL, 1, 0);

    // 4. Start / Stop Button
    m_btnStart = CreateWindowExW(
        0, WC_BUTTONW, L"Run",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        635, 19, 105, 26,
        m_hWnd, reinterpret_cast<HMENU>(ID_BTN_START), hInst, nullptr
    );
    SendMessageW(m_btnStart, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontTitle), TRUE);

    PopulateDrives();

    // 30 FPS update timer
    SetTimer(m_hWnd, 1001, 33, nullptr);
}

void MainWindow::PopulateDrives() {
    m_drives = DetectAllDrives();
    SendMessageW(m_cbDrives, CB_RESETCONTENT, 0, 0);

    for (size_t i = 0; i < m_drives.size(); ++i) {
        std::wstring name = m_drives[i].GetDisplayName();
        SendMessageW(m_cbDrives, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
    }

    SendMessageW(m_cbDrives, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Browse Custom Folder..."));

    if (!m_drives.empty()) {
        SendMessageW(m_cbDrives, CB_SETCURSEL, 0, 0);
        m_selectedDriveIndex = 0;
    }
}

void MainWindow::PromptCustomDirectory() {
    BROWSEINFOW bi = {0};
    bi.hwndOwner = m_hWnd;
    bi.lpszTitle = L"Select Storage Directory or Drive";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        wchar_t path[MAX_PATH] = {0};
        if (SHGetPathFromIDListW(pidl, path)) {
            m_customPath = path;
            if (m_customPath.back() != L'\\') m_customPath += L'\\';

            DriveInfo customInfo;
            customInfo.path = m_customPath;
            customInfo.volumeName = L"Custom";
            customInfo.fileSystem = L"Path";
            customInfo.driveType = L"Folder";

            ULARGE_INTEGER freeBytes, totalBytes;
            if (GetDiskFreeSpaceExW(m_customPath.c_str(), &freeBytes, &totalBytes, nullptr)) {
                customInfo.freeBytes = freeBytes.QuadPart;
                customInfo.totalBytes = totalBytes.QuadPart;
            }
            m_drives.push_back(customInfo);

            int newIdx = static_cast<int>(m_drives.size() - 1);
            SendMessageW(m_cbDrives, CB_INSERTSTRING, newIdx, reinterpret_cast<LPARAM>(customInfo.GetDisplayName().c_str()));
            SendMessageW(m_cbDrives, CB_SETCURSEL, newIdx, 0);
            m_selectedDriveIndex = newIdx;
        }
        CoTaskMemFree(pidl);
    } else {
        SendMessageW(m_cbDrives, CB_SETCURSEL, m_selectedDriveIndex, 0);
    }
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

void MainWindow::OnPaint() {
    RECT rc;
    GetClientRect(m_hWnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    CreateMemBuffer(width, height);

    Graphics g(m_hMemDC);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    // Deep Dark Base Background (#0D1117)
    SolidBrush bgBrush(Color(255, 13, 17, 23));
    g.FillRectangle(&bgBrush, 0, 0, width, height);

    // Neat Material 3 Cards (#161B22 with #30363D border)
    Color cardBg(255, 22, 27, 34);
    Color cardBorder(255, 48, 54, 61);
    SolidBrush brushCard(cardBg);
    Pen penCard(cardBorder, 1.0f);

    Font fontSpeed(L"Segoe UI", 36.0f, FontStyleBold, UnitPoint);
    Font fontSpeedUnit(L"Segoe UI", 13.0f, FontStyleBold, UnitPoint);
    Font fontStatus(L"Segoe UI", 9.5f, FontStyleBold, UnitPoint);
    Font fontRegular(L"Segoe UI", 9.5f, FontStyleRegular, UnitPoint);
    Font fontBold(L"Segoe UI", 9.5f, FontStyleBold, UnitPoint);
    Font fontSmall(L"Segoe UI", 8.5f, FontStyleRegular, UnitPoint);

    SolidBrush brushWhite(Color(255, 230, 237, 243));
    SolidBrush brushMuted(Color(255, 139, 148, 158));
    SolidBrush brushCyan(Color(255, 88, 166, 255));      // #58A6FF
    SolidBrush brushEmerald(Color(255, 63, 185, 80));    // #3FB950
    SolidBrush brushRose(Color(255, 248, 81, 73));       // #F85149

    LiveStatus status = m_engine.GetStatus();

    // 1. Hero Speed Display Card (Y: 60, H: 86)
    RectF rectSpeedCard(20.0f, 60.0f, width - 40.0f, 86.0f);
    FillRoundedRect(g, brushCard, rectSpeedCard, 6.0f);
    DrawRoundedRect(g, penCard, rectSpeedCard, 6.0f);

    std::wostringstream speedValStr;
    speedValStr << std::fixed << std::setprecision(1) << status.instantMBps;
    std::wstring speedNum = speedValStr.str();

    PointF ptSpeed(36.0f, 68.0f);
    g.DrawString(speedNum.c_str(), -1, &fontSpeed, ptSpeed, &brushCyan);

    // Speed unit right next to the number
    RectF boundRect;
    g.MeasureString(speedNum.c_str(), -1, &fontSpeed, ptSpeed, &boundRect);
    PointF ptUnit(boundRect.Right() + 4.0f, 84.0f);
    g.DrawString(L"MB/s", -1, &fontSpeedUnit, ptUnit, &brushMuted);

    // Status label below speed
    std::wstring statusLabel = L"\u25CF Ready";
    Color statusCol = Color(255, 139, 148, 158);
    if (status.isRunning) {
        statusLabel = L"\u25CF " + std::wstring(status.activeTest.begin(), status.activeTest.end());
        statusCol = Color(255, 63, 185, 80);
    } else if (status.phase == "Completed") {
        statusLabel = L"\u25CF Completed";
        statusCol = Color(255, 63, 185, 80);
    } else if (status.phase == "Aborted") {
        statusLabel = L"\u25CF Aborted";
        statusCol = Color(255, 248, 81, 73);
    }
    SolidBrush brushStatus(statusCol);
    PointF ptStatus(38.0f, 118.0f);
    g.DrawString(statusLabel.c_str(), -1, &fontStatus, ptStatus, &brushStatus);

    // Peak & Average text on the right side of hero card
    std::wostringstream rightStats;
    rightStats << L"Peak: " << std::fixed << std::setprecision(1) << status.peakMBps << L" MB/s   "
               << L"Avg: " << std::fixed << std::setprecision(1) << status.averageMBps << L" MB/s   "
               << L"IOPS: " << status.currentIOPS;
    std::wstring rStatsW = rightStats.str();
    PointF ptRStats(width - 340.0f, 96.0f);
    g.DrawString(rStatsW.c_str(), -1, &fontRegular, ptRStats, &brushMuted);

    // Progress bar inside hero card bottom
    float pbX = 36.0f;
    float pbY = 138.0f;
    float pbW = width - 72.0f;
    float pbH = 3.0f;
    SolidBrush brushPbTrack(Color(255, 33, 38, 45));
    RectF rectPbTrack(pbX, pbY, pbW, pbH);
    FillRoundedRect(g, brushPbTrack, rectPbTrack, 1.5f);

    float fillW = (pbW * (status.progress / 100.0f));
    if (fillW > 2.0f) {
        SolidBrush brushPbFill(Color(255, 88, 166, 255));
        RectF rectPbFill(pbX, pbY, fillW, pbH);
        FillRoundedRect(g, brushPbFill, rectPbFill, 1.5f);
    }

    // 2. Real-Time Throughput Graph Card (Y: 156, H: 140)
    RectF rectChartCard(20.0f, 156.0f, width - 40.0f, 140.0f);
    FillRoundedRect(g, brushCard, rectChartCard, 6.0f);
    DrawRoundedRect(g, penCard, rectChartCard, 6.0f);

    float plotX = 34.0f;
    float plotY = 170.0f;
    float plotW = width - 68.0f;
    float plotH = 110.0f;

    // Grid lines
    Pen penGrid(Color(255, 33, 38, 45), 1.0f);
    for (int i = 0; i <= 2; ++i) {
        float gy = plotY + (plotH / 2.0f) * i;
        g.DrawLine(&penGrid, plotX, gy, plotX + plotW, gy);
    }

    double maxSpeed = 100.0;
    for (const auto& smp : status.history) {
        if (smp.instantMBps > maxSpeed) maxSpeed = smp.instantMBps;
    }
    maxSpeed *= 1.15;

    std::wostringstream maxSpd;
    maxSpd << std::fixed << std::setprecision(0) << maxSpeed << L" MB/s";
    PointF ptMaxLabel(plotX + 4.0f, plotY - 12.0f);
    g.DrawString(maxSpd.str().c_str(), -1, &fontSmall, ptMaxLabel, &brushMuted);

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

        // Area under curve
        GraphicsPath fillPath;
        fillPath.AddLine(plotX, plotY + plotH, pts[0].X, pts[0].Y);
        for (size_t i = 1; i < pts.size(); ++i) {
            fillPath.AddLine(pts[i - 1], pts[i]);
        }
        fillPath.AddLine(pts.back().X, plotY + plotH);
        fillPath.CloseFigure();

        SolidBrush areaBrush(Color(35, 88, 166, 255));
        g.FillPath(&areaBrush, &fillPath);

        // Curve line
        Pen penCurve(Color(255, 88, 166, 255), 1.8f);
        for (size_t i = 1; i < pts.size(); ++i) {
            g.DrawLine(&penCurve, pts[i - 1], pts[i]);
        }

        // Glowing head point
        PointF head = pts.back();
        SolidBrush brushHead(Color(255, 88, 166, 255));
        g.FillEllipse(&brushHead, head.X - 3.0f, head.Y - 3.0f, 6.0f, 6.0f);
    }

    // 3. Neat Results Table Card (Y: 306, H: 180)
    RectF rectTableCard(20.0f, 306.0f, width - 40.0f, 180.0f);
    FillRoundedRect(g, brushCard, rectTableCard, 6.0f);
    DrawRoundedRect(g, penCard, rectTableCard, 6.0f);

    // Table Header
    float thY = 318.0f;
    g.DrawString(L"Test Routine", -1, &fontBold, PointF(36.0f, thY), &brushMuted);
    g.DrawString(L"Average Speed", -1, &fontBold, PointF(220.0f, thY), &brushMuted);
    g.DrawString(L"Peak Speed", -1, &fontBold, PointF(380.0f, thY), &brushMuted);
    g.DrawString(L"IOPS", -1, &fontBold, PointF(520.0f, thY), &brushMuted);
    g.DrawString(L"Latency", -1, &fontBold, PointF(640.0f, thY), &brushMuted);

    Pen penDiv(Color(255, 33, 38, 45), 1.0f);
    g.DrawLine(&penDiv, 32.0f, thY + 22.0f, width - 32.0f, thY + 22.0f);

    if (status.completedResults.empty()) {
        PointF ptEmpty(36.0f, thY + 36.0f);
        g.DrawString(L"No results yet. Select drive and click 'Run'.", -1, &fontRegular, ptEmpty, &brushMuted);
    } else {
        float rowY = thY + 30.0f;
        for (const auto& r : status.completedResults) {
            std::wstring wName(r.testName.begin(), r.testName.end());
            g.DrawString(wName.c_str(), -1, &fontBold, PointF(36.0f, rowY), &brushWhite);

            std::wostringstream ssAvg, ssPeak, ssIOPS, ssLat;
            ssAvg << std::fixed << std::setprecision(1) << r.avgMBps << L" MB/s";
            ssPeak << std::fixed << std::setprecision(1) << r.peakMBps << L" MB/s";
            ssIOPS << r.avgIOPS;
            ssLat << std::fixed << std::setprecision(3) << r.avgLatencyMs << L" ms";

            g.DrawString(ssAvg.str().c_str(), -1, &fontBold, PointF(220.0f, rowY), &brushCyan);
            g.DrawString(ssPeak.str().c_str(), -1, &fontRegular, PointF(380.0f, rowY), &brushEmerald);
            g.DrawString(ssIOPS.str().c_str(), -1, &fontRegular, PointF(520.0f, rowY), &brushWhite);
            g.DrawString(ssLat.str().c_str(), -1, &fontRegular, PointF(640.0f, rowY), &brushMuted);

            rowY += 28.0f;
        }
    }

    // Blit to screen
    HDC hdcScreen = GetDC(m_hWnd);
    BitBlt(hdcScreen, 0, 0, width, height, m_hMemDC, 0, 0, SRCCOPY);
    ReleaseDC(m_hWnd, hdcScreen);
}

void MainWindow::OnTimer() {
    LiveStatus status = m_engine.GetStatus();

    if (status.isRunning) {
        SetWindowTextW(m_btnStart, L"Stop");
        EnableWindow(m_cbDrives, FALSE);
        EnableWindow(m_cbMode, FALSE);
        EnableWindow(m_cbSize, FALSE);
    } else {
        SetWindowTextW(m_btnStart, L"Run");
        EnableWindow(m_cbDrives, TRUE);
        EnableWindow(m_cbMode, TRUE);
        EnableWindow(m_cbSize, TRUE);
    }

    RECT rc;
    GetClientRect(m_hWnd, &rc);
    rc.top = 56; // Repaint only display cards below controls
    InvalidateRect(m_hWnd, &rc, FALSE);
}

void MainWindow::OnCommand(WORD id, WORD code) {
    if (id == ID_CB_DRIVES && code == CBN_SELCHANGE) {
        int idx = static_cast<int>(SendMessageW(m_cbDrives, CB_GETCURSEL, 0, 0));
        if (idx == static_cast<int>(m_drives.size())) {
            PromptCustomDirectory();
        } else {
            m_selectedDriveIndex = idx;
        }
    } else if (id == ID_BTN_START && code == BN_CLICKED) {
        HandleStartStop();
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

    int modeIdx = static_cast<int>(SendMessageW(m_cbMode, CB_GETCURSEL, 0, 0));
    TestMode mode = TestMode::FullSuite;
    if (modeIdx == 1) mode = TestMode::SequentialWrite;
    else if (modeIdx == 2) mode = TestMode::SequentialRead;
    else if (modeIdx == 3) mode = TestMode::Random4KWrite;

    int sizeIdx = static_cast<int>(SendMessageW(m_cbSize, CB_GETCURSEL, 0, 0));
    uint64_t targetSize = 256ULL * 1024ULL * 1024ULL;
    if (sizeIdx == 0) targetSize = 128ULL * 1024ULL * 1024ULL;
    else if (sizeIdx == 1) targetSize = 256ULL * 1024ULL * 1024ULL;
    else if (sizeIdx == 2) targetSize = 512ULL * 1024ULL * 1024ULL;
    else if (sizeIdx == 3) targetSize = 1024ULL * 1024ULL * 1024ULL;

    m_engine.Start(targetPath, mode, targetSize, 1048576);
}

void MainWindow::OnDestroy() {
    KillTimer(m_hWnd, 1001);
    m_engine.Abort();
}
