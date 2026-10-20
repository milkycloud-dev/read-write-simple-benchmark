#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <gdiplus.h>
#include <iostream>
#include <string>
#include "benchmark.hpp"
#include "drives.hpp"
#include "ui.hpp"

#pragma comment(lib, "gdiplus.lib")

static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;
    Gdiplus::ImageCodecInfo* pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (!pImageCodecInfo) return -1;
    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }
    free(pImageCodecInfo);
    return -1;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);

    // Enable DPI awareness for crisp native rendering
    SetProcessDPIAware();

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(pCmdLine, &argc);
    if (argv) {
        for (int i = 0; i < argc; ++i) {
            std::wstring arg = argv[i];
            if (arg == L"--cli" || arg == L"-c") {
                if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()) {
                    FILE* fp = nullptr;
                    freopen_s(&fp, "CONOUT$", "w", stdout);
                    std::cout << "=================================================\n";
                    std::cout << "  RW-BENCH - Disk Speed Benchmark\n";
                    std::cout << "=================================================\n\n";

                    std::cout << "Detecting storage drives...\n";
                    auto drives = DetectAllDrives();
                    for (size_t d = 0; d < drives.size(); ++d) {
                        std::wcout << L"[" << d << L"] " << drives[d].GetDisplayName() << L"\n";
                    }
                    LocalFree(argv);
                    return 0;
                }
            } else if ((arg == L"--screenshot" || arg == L"-s") && (i + 1 < argc)) {
                std::wstring outPath = argv[i + 1];

                Gdiplus::GdiplusStartupInput gdiplusStartupInput;
                ULONG_PTR gdiplusToken = 0;
                Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
                {
                    BenchmarkEngine engine;
                    MainWindow win(engine);
                    win.PopulateDrives();

                    int w = 640;
                    int h = 404;
                    Gdiplus::Bitmap bmp(w, h, PixelFormat32bppARGB);
                    Gdiplus::Graphics g(&bmp);
                    win.Render(g, w, h);

                    CLSID pngClsid;
                    if (GetEncoderClsid(L"image/png", &pngClsid) >= 0) {
                        bmp.Save(outPath.c_str(), &pngClsid, nullptr);
                    }
                }
                Gdiplus::GdiplusShutdown(gdiplusToken);
                LocalFree(argv);
                return 0;
            }
        }
        LocalFree(argv);
    }

    // Initialize GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken = 0;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

    BenchmarkEngine engine;
    MainWindow mainWindow(engine);

    if (!mainWindow.Create()) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        MessageBoxW(nullptr, L"Failed to create native desktop window.", L"RW-Bench Error", MB_ICONERROR);
        return 1;
    }

    mainWindow.Show(nCmdShow);

    // Standard Win32 Message Loop
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Clean up GDI+
    Gdiplus::GdiplusShutdown(gdiplusToken);

    return static_cast<int>(msg.wParam);
}
