#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <memory>
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

    void OnCreate();
    void OnPaint();
    void OnTimer();
    void OnCommand(WORD id, WORD code);
    void OnDestroy();

    void PopulateDrives();
    void HandleStartStop();
    void PromptCustomDirectory();

    BenchmarkEngine& m_engine;
    HWND m_hWnd = nullptr;
    HWND m_cbDrives = nullptr;
    HWND m_cbSize = nullptr;
    HWND m_cbMode = nullptr;
    HWND m_btnStart = nullptr;
    HWND m_btnCopy = nullptr;

    HFONT m_hFontUI = nullptr;
    HFONT m_hFontTitle = nullptr;
    HFONT m_hFontSpeed = nullptr;
    HFONT m_hFontSmall = nullptr;

    std::vector<DriveInfo> m_drives;
    int m_selectedDriveIndex = 0;
    std::wstring m_customPath;

    // Double buffering memory DC & Bitmap
    HDC m_hMemDC = nullptr;
    HBITMAP m_hMemBitmap = nullptr;
    HBITMAP m_hOldBitmap = nullptr;
    int m_memWidth = 0;
    int m_memHeight = 0;

    void CreateMemBuffer(int width, int height);
    void CleanupMemBuffer();
};
