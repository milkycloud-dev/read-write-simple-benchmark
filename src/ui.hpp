#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include "benchmark.hpp"
#include "drives.hpp"

class MainWindow {
public:
    MainWindow(BenchmarkEngine& engine);
    ~MainWindow();

    bool Create();
    void Show(int nCmdShow);
    HWND GetHwnd() const { return m_hWnd; }

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void OnPaint();
    void OnTimer();
    void OnDestroy();

    void PopulateDrives();
    void HandleStartStop();
    void PromptCustomDirectory();
    void ShowDriveMenu();
    void ShowModeMenu();
    void ShowSizeMenu();
    int HitTest(int x, int y) const;

    BenchmarkEngine& m_engine;
    HWND m_hWnd = nullptr;

    std::vector<DriveInfo> m_drives;
    int m_selectedDriveIndex = 0;
    std::wstring m_customPath;

    // 0: Full Suite, 1: Seq Write, 2: Seq Read, 3: Random 4K
    int m_selectedMode = 0;
    // 0: 128 MB, 1: 256 MB, 2: 512 MB, 3: 1 GB
    int m_selectedSize = 1;

    // Hover state: 0=none, 1=mode, 2=size, 3=drive, 4=start/stop
    int m_hoverId = 0;
    bool m_trackingMouse = false;

    // Saved live histories for Write and Read cards
    std::vector<BenchmarkSample> m_writeHistory;
    std::vector<BenchmarkSample> m_readHistory;

    // Double buffering
    HDC m_hMemDC = nullptr;
    HBITMAP m_hMemBitmap = nullptr;
    HBITMAP m_hOldBitmap = nullptr;
    int m_memWidth = 0;
    int m_memHeight = 0;

    void CreateMemBuffer(int width, int height);
    void CleanupMemBuffer();
};
