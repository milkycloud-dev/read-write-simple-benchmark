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

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);

    // Support --cli flag for headless testing or scripting
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(pCmdLine, &argc);
    if (argv) {
        for (int i = 0; i < argc; ++i) {
            if (std::wstring(argv[i]) == L"--cli" || std::wstring(argv[i]) == L"-c") {
                // Attach or allocate console if requested via CLI
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
