#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "ui.hpp"
#include <gdiplus.h>
#include <dwmapi.h>
#include <shlobj.h>
#include <commctrl.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

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
    ID_BTN_START = 204,
    ID_BTN_COPY  = 205
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
    path.AddArc(rect.Right() - radius * 2, rect.Bottom() - radius * 2, radius * 2, radius * 2, 0, 90);
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
    wc.lpszClassName = L"RWBenchNativeWindowClass";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(15, 18, 24)); // Dark base background
    wc.style = CS_HREDRAW | CS_VREDRAW;

    RegisterClassExW(&wc);

    int winW = 960;
    int winH = 680;
    int posX = (GetSystemMetrics(SM_CXSCREEN) - winW) / 2;
    int posY = (GetSystemMetrics(SM_CYSCREEN) - winH) / 2;

    m_hWnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"RW-BENCH \u2022 Disk Speed Benchmark (Direct I/O)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY, winW, winH,
        nullptr, nullptr, wc.hInstance, this
    );

    if (!m_hWnd) return false;

    // Enable Windows 10/11 Immersive Dark Mode for Title Bar
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
            return 1; // Handled in double buffer paint

        case WM_TIMER:
            OnTimer();
            return 0;

        case WM_COMMAND:
            OnCommand(LOWORD(wParam), HIWORD(wParam));
            return 0;

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetTextColor(hdc, RGB(220, 230, 245));
            SetBkColor(hdc, RGB(22, 27, 36));
            static HBRUSH hBr = CreateSolidBrush(RGB(22, 27, 36));
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
    // Crisp Native Windows Font (Segoe UI)
    m_hFontUI    = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hFontTitle = CreateFontW(17, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hFontSpeed = CreateFontW(44, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hFontSmall = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    HINSTANCE hInst = GetModuleHandleW(nullptr);

    // 1. Drive Selection ComboBox
    m_cbDrives = CreateWindowExW(
        0, WC_COMBOBOXW, L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
        24, 30, 390, 260,
        m_hWnd, reinterpret_cast<HMENU>(ID_CB_DRIVES), hInst, nullptr
    );
    SendMessageW(m_cbDrives, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontUI), TRUE);

    // 2. Routine Mode ComboBox
    m_cbMode = CreateWindowExW(
        0, WC_COMBOBOXW, L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
        426, 30, 160, 200,
        m_hWnd, reinterpret_cast<HMENU>(ID_CB_MODE), hInst, nullptr
    );
    SendMessageW(m_cbMode, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontUI), TRUE);
    SendMessageW(m_cbMode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Full Suite (All 4)"));
    SendMessageW(m_cbMode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Sequential Write"));
    SendMessageW(m_cbMode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Sequential Read"));
    SendMessageW(m_cbMode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Random 4K (IOPS)"));
    SendMessageW(m_cbMode, CB_SETCURSEL, 0, 0);

    // 3. Size ComboBox
    m_cbSize = CreateWindowExW(
        0, WC_COMBOBOXW, L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
        598, 30, 110, 200,
        m_hWnd, reinterpret_cast<HMENU>(ID_CB_SIZE), hInst, nullptr
    );
    SendMessageW(m_cbSize, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFontUI), TRUE);
    SendMessageW(m_cbSize, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"128 MB"));
    SendMessageW(m_cbSize, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"256 MB"));
    SendMessageW(m_cbSize, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"512 MB"));
    SendMessageW(m_cbSize, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"1 GB"));
    SendMessageW(m_cbSize, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"2 GB"));
    SendMessageW(m_cbSize, CB_SETCURSEL, 1, 0); // 256 MB default

    // 4. Start Button
    m_btnStart = CreateWindowExW(
        0, WC_BUTTONW, L"START BENCHMARK",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        720, 28, 200, 34,
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

    SendMessageW(m_cbDrives, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Browse Custom Folder / Drive..."));

    if (!m_drives.empty()) {
        SendMessageW(m_cbDrives, CB_SETCURSEL, 0, 0);
        m_selectedDriveIndex = 0;
    }
}

void MainWindow::PromptCustomDirectory() {
    BROWSEINFOW bi = {0};
    bi.hwndOwner = m_hWnd;
    bi.lpszTitle = L"Select Storage Directory or Drive for Benchmark";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        wchar_t path[MAX_PATH] = {0};
        if (SHGetPathFromIDListW(pidl, path)) {
            m_customPath = path;
            if (m_customPath.back() != L'\\') m_customPath += L'\\';

            DriveInfo customInfo;
            customInfo.path = m_customPath;
            customInfo.volumeName = L"Custom Folder";
            customInfo.fileSystem = L"Selected Path";
            customInfo.driveType = L"Directory";

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

    // Deep Dark Material Base
    SolidBrush bgBrush(Color(255, 12, 15, 20)); // #0C0F14
    g.FillRectangle(&bgBrush, 0, 0, width, height);

    // Card Colors
    Color cardBg(255, 18, 23, 31);       // #12171F
    Color cardBorder(255, 34, 42, 56);   // #222A38
    SolidBrush brushCard(cardBg);
    Pen penCard(cardBorder, 1.0f);

    Font fontUI(L"Segoe UI", 10.0f, FontStyleRegular, UnitPoint);
    Font fontLabel(L"Segoe UI", 8.5f, FontStyleBold, UnitPoint);
    Font fontSpeed(L"Segoe UI", 34.0f, FontStyleBold, UnitPoint);
    Font fontBadge(L"Segoe UI", 9.0f, FontStyleBold, UnitPoint);
    Font fontTable(L"Segoe UI", 9.5f, FontStyleRegular, UnitPoint);
    Font fontTableBold(L"Segoe UI", 9.5f, FontStyleBold, UnitPoint);

    SolidBrush brushWhite(Color(255, 245, 248, 255));
    SolidBrush brushMuted(Color(255, 130, 145, 170));
    SolidBrush brushCyan(Color(255, 0, 229, 255));
    SolidBrush brushEmerald(Color(255, 0, 230, 118));
    SolidBrush brushRose(Color(255, 255, 23, 68));

    // Top Controls Card
    RectF rectControls(16.0f, 12.0f, width - 32.0f, 62.0f);
    FillRoundedRect(g, brushCard, rectControls, 8.0f);
    DrawRoundedRect(g, penCard, rectControls, 8.0f);

    // Labels above comboboxes
    PointF ptLblDrive(24.0f, 16.0f);
    g.DrawString(L"STORAGE DRIVE", -1, &fontLabel, ptLblDrive, &brushMuted);

    PointF ptLblMode(426.0f, 16.0f);
    g.DrawString(L"BENCHMARK ROUTINE", -1, &fontLabel, ptLblMode, &brushMuted);

    PointF ptLblSize(598.0f, 16.0f);
    g.DrawString(L"TEST SIZE", -1, &fontLabel, ptLblSize, &brushMuted);

    LiveStatus status = m_engine.GetStatus();

    // 1. Live Speed & Phase Card
    RectF rectSpeedCard(16.0f, 84.0f, width - 32.0f, 130.0f);
    FillRoundedRect(g, brushCard, rectSpeedCard, 8.0f);
    DrawRoundedRect(g, penCard, rectSpeedCard, 8.0f);

    // Digital Speed Display
    std::wostringstream speedStr;
    speedStr << std::fixed << std::setprecision(1) << status.instantMBps << L" MB/s";
    std::wstring speedW = speedStr.str();

    PointF ptSpeed(32.0f, 96.0f);
    g.DrawString(speedW.c_str(), -1, &fontSpeed, ptSpeed, &brushCyan);

    // Active Phase Badge
    std::wstring phaseText = L"IDLE / READY";
    Color badgeColor = Color(255, 130, 145, 170);
    if (status.isRunning) {
        if (!status.activeTest.empty()) {
            phaseText = std::wstring(status.activeTest.begin(), status.activeTest.end());
            badgeColor = Color(255, 0, 230, 118);
        } else {
            phaseText = L"RUNNING";
            badgeColor = Color(255, 0, 229, 255);
        }
    } else if (status.phase == "Completed") {
        phaseText = L"COMPLETED";
        badgeColor = Color(255, 0, 230, 118);
    } else if (status.phase == "Aborted") {
        phaseText = L"ABORTED";
        badgeColor = Color(255, 255, 23, 68);
    }

    SolidBrush brushBadgeText(badgeColor);
    PointF ptBadge(34.0f, 150.0f);
    g.DrawString(phaseText.c_str(), -1, &fontBadge, ptBadge, &brushBadgeText);

    // Key stats row
    std::wostringstream statsRow;
    statsRow << L"Peak: " << std::fixed << std::setprecision(1) << status.peakMBps << L" MB/s    "
             << L"Average: " << std::fixed << std::setprecision(1) << status.averageMBps << L" MB/s    "
             << L"IOPS: " << status.currentIOPS << L"    "
             << L"Elapsed: " << std::fixed << std::setprecision(1) << status.elapsedSeconds << L" s";
    std::wstring statsW = statsRow.str();
    PointF ptStats(34.0f, 176.0f);
    g.DrawString(statsW.c_str(), -1, &fontUI, ptStats, &brushMuted);

    // Progress Bar
    float pbX = 32.0f;
    float pbY = 200.0f;
    float pbW = width - 64.0f;
    float pbH = 6.0f;

    SolidBrush brushPbTrack(Color(255, 28, 35, 48));
    RectF rectPbTrack(pbX, pbY, pbW, pbH);
    FillRoundedRect(g, brushPbTrack, rectPbTrack, 3.0f);

    float fillW = (pbW * (status.progress / 100.0f));
    if (fillW > 4.0f) {
        LinearGradientBrush pbGradient(PointF(pbX, pbY), PointF(pbX + pbW, pbY), Color(255, 0, 229, 255), Color(255, 0, 230, 118));
        RectF rectPbFill(pbX, pbY, fillW, pbH);
        FillRoundedRect(g, pbGradient, rectPbFill, 3.0f);
    }

    // 2. Real-Time Throughput Graph Card
    float chartCardY = 224.0f;
    float chartCardH = 200.0f;
    RectF rectChartCard(16.0f, chartCardY, width - 32.0f, chartCardH);
    FillRoundedRect(g, brushCard, rectChartCard, 8.0f);
    DrawRoundedRect(g, penCard, rectChartCard, 8.0f);

    PointF ptChartTitle(26.0f, chartCardY + 10.0f);
    g.DrawString(L"REAL-TIME THROUGHPUT STREAM (Direct I/O)", -1, &fontLabel, ptChartTitle, &brushMuted);

    // Chart plotting area
    float plotX = 32.0f;
    float plotY = chartCardY + 36.0f;
    float plotW = width - 64.0f;
    float plotH = 148.0f;

    // Grid lines
    Pen penGrid(Color(255, 26, 33, 46), 1.0f);
    for (int i = 0; i <= 3; ++i) {
        float gy = plotY + (plotH / 3.0f) * i;
        g.DrawLine(&penGrid, plotX, gy, plotX + plotW, gy);
    }

    // Determine max speed scale
    double maxSpeed = 100.0;
    for (const auto& smp : status.history) {
        if (smp.instantMBps > maxSpeed) maxSpeed = smp.instantMBps;
        if (smp.averageMBps > maxSpeed) maxSpeed = smp.averageMBps;
    }
    maxSpeed *= 1.15; // 15% headroom

    // Draw max speed label
    std::wostringstream maxSpeedStr;
    maxSpeedStr << std::fixed << std::setprecision(0) << maxSpeed << L" MB/s";
    PointF ptMaxLabel(plotX + 6.0f, plotY + 2.0f);
    g.DrawString(maxSpeedStr.str().c_str(), -1, &fontSmall, ptMaxLabel, &brushMuted);

    // Plot data points
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

        // Fill gradient area under the curve
        GraphicsPath fillPath;
        fillPath.AddLine(plotX, plotY + plotH, pts[0].X, pts[0].Y);
        for (size_t i = 1; i < pts.size(); ++i) {
            fillPath.AddLine(pts[i - 1], pts[i]);
        }
        fillPath.AddLine(pts.back().X, plotY + plotH);
        fillPath.CloseFigure();

        LinearGradientBrush areaBrush(PointF(plotX, plotY), PointF(plotX, plotY + plotH), Color(70, 0, 229, 255), Color(0, 0, 229, 255));
        g.FillPath(&areaBrush, &fillPath);

        // Instant curve line
        Pen penInstant(Color(255, 0, 229, 255), 2.2f);
        for (size_t i = 1; i < pts.size(); ++i) {
            g.DrawLine(&penInstant, pts[i - 1], pts[i]);
        }

        // Glowing head dot
        PointF head = pts.back();
        SolidBrush brushHead(Color(255, 0, 229, 255));
        g.FillEllipse(&brushHead, head.X - 4.0f, head.Y - 4.0f, 8.0f, 8.0f);
    }

    // 3. Compact Results Table Card
    float tableCardY = 434.0f;
    float tableCardH = 190.0f;
    RectF rectTableCard(16.0f, tableCardY, width - 32.0f, tableCardH);
    FillRoundedRect(g, brushCard, rectTableCard, 8.0f);
    DrawRoundedRect(g, penCard, rectTableCard, 8.0f);

    PointF ptTableTitle(26.0f, tableCardY + 10.0f);
    g.DrawString(L"BENCHMARK RESULTS SUMMARY", -1, &fontLabel, ptTableTitle, &brushMuted);

    // Table Header
    float thY = tableCardY + 34.0f;
    g.DrawString(L"Routine Test", -1, &fontTableBold, PointF(34.0f, thY), &brushMuted);
    g.DrawString(L"Average Speed", -1, &fontTableBold, PointF(260.0f, thY), &brushMuted);
    g.DrawString(L"Peak Speed", -1, &fontTableBold, PointF(440.0f, thY), &brushMuted);
    g.DrawString(L"IOPS", -1, &fontTableBold, PointF(620.0f, thY), &brushMuted);
    g.DrawString(L"Avg Latency", -1, &fontTableBold, PointF(780.0f, thY), &brushMuted);

    Pen penDivider(Color(255, 30, 38, 52), 1.0f);
    g.DrawLine(&penDivider, 30.0f, thY + 20.0f, width - 30.0f, thY + 20.0f);

    if (status.completedResults.empty()) {
        PointF ptEmpty(34.0f, thY + 36.0f);
        g.DrawString(L"No completed tests yet. Select drive and routine, then click START BENCHMARK.", -1, &fontTable, ptEmpty, &brushMuted);
    } else {
        float rowY = thY + 28.0f;
        for (const auto& r : status.completedResults) {
            std::wstring wTestName(r.testName.begin(), r.testName.end());
            g.DrawString(wTestName.c_str(), -1, &fontTableBold, PointF(34.0f, rowY), &brushWhite);

            std::wostringstream ssAvg, ssPeak, ssIOPS, ssLat;
            ssAvg << std::fixed << std::setprecision(1) << r.avgMBps << L" MB/s";
            ssPeak << std::fixed << std::setprecision(1) << r.peakMBps << L" MB/s";
            ssIOPS << r.avgIOPS;
            ssLat << std::fixed << std::setprecision(3) << r.avgLatencyMs << L" ms";

            g.DrawString(ssAvg.str().c_str(), -1, &fontTable, PointF(260.0f, rowY), &brushCyan);
            g.DrawString(ssPeak.str().c_str(), -1, &fontTable, PointF(440.0f, rowY), &brushEmerald);
            g.DrawString(ssIOPS.str().c_str(), -1, &fontTable, PointF(620.0f, rowY), &brushWhite);
            g.DrawString(ssLat.str().c_str(), -1, &fontTable, PointF(780.0f, rowY), &brushMuted);

            rowY += 26.0f;
        }
    }

    // Blit to screen
    HDC hdcScreen = GetDC(m_hWnd);
    BitBlt(hdcScreen, 0, 0, width, height, m_hMemDC, 0, 0, SRCCOPY);
    ReleaseDC(m_hWnd, hdcScreen);
}

void MainWindow::OnTimer() {
    LiveStatus status = m_engine.GetStatus();

    // Update Start / Abort button text and state
    if (status.isRunning) {
        SetWindowTextW(m_btnStart, L"ABORT BENCHMARK");
        EnableWindow(m_cbDrives, FALSE);
        EnableWindow(m_cbMode, FALSE);
        EnableWindow(m_cbSize, FALSE);
    } else {
        SetWindowTextW(m_btnStart, L"START BENCHMARK");
        EnableWindow(m_cbDrives, TRUE);
        EnableWindow(m_cbMode, TRUE);
        EnableWindow(m_cbSize, TRUE);
    }

    // Invalidate main display area to repaint 60fps graph
    RECT rc;
    GetClientRect(m_hWnd, &rc);
    rc.top = 80; // Only repaint custom area below controls
    InvalidateRect(m_hWnd, &rc, FALSE);
}

void MainWindow::OnCommand(WORD id, WORD code) {
    if (id == ID_CB_DRIVES && code == CBN_SELCHANGE) {
        int idx = static_cast<int>(SendMessageW(m_cbDrives, CB_GETCURSEL, 0, 0));
        if (idx == static_cast<int>(m_drives.size())) {
            // Selected "Browse Custom Folder / Drive..."
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

    // Routine Mode
    int modeIdx = static_cast<int>(SendMessageW(m_cbMode, CB_GETCURSEL, 0, 0));
    TestMode mode = TestMode::FullSuite;
    if (modeIdx == 1) mode = TestMode::SequentialWrite;
    else if (modeIdx == 2) mode = TestMode::SequentialRead;
    else if (modeIdx == 3) mode = TestMode::Random4KWrite;

    // File Size
    int sizeIdx = static_cast<int>(SendMessageW(m_cbSize, CB_GETCURSEL, 0, 0));
    uint64_t targetSize = 256ULL * 1024ULL * 1024ULL; // 256 MB default
    if (sizeIdx == 0) targetSize = 128ULL * 1024ULL * 1024ULL;
    else if (sizeIdx == 1) targetSize = 256ULL * 1024ULL * 1024ULL;
    else if (sizeIdx == 2) targetSize = 512ULL * 1024ULL * 1024ULL;
    else if (sizeIdx == 3) targetSize = 1024ULL * 1024ULL * 1024ULL;
    else if (sizeIdx == 4) targetSize = 2048ULL * 1024ULL * 1024ULL;

    m_engine.Start(targetPath, mode, targetSize, 1048576);
}

void MainWindow::OnDestroy() {
    KillTimer(m_hWnd, 1001);
    m_engine.Abort();
}
