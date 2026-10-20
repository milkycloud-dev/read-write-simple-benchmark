#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "benchmark.hpp"
#include "drives.hpp"
#include "server.hpp"

#include <windows.h>
#include <shellapi.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

static std::wstring FindBrowserExecutable() {
    // 1. Google Chrome (Preferred for App Mode)
    const wchar_t* chromePaths[] = {
        L"C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe",
        L"C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe"
    };
    for (const auto* path : chromePaths) {
        if (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES) {
            return path;
        }
    }

    // 2. Microsoft Edge (Supports --app mode)
    const wchar_t* edgePaths[] = {
        L"C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe",
        L"C:\\Program Files\\Microsoft\\Edge\\Application\\msedge.exe"
    };
    for (const auto* path : edgePaths) {
        if (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES) {
            return path;
        }
    }

    return L"";
}

int main(int argc, char* argv[]) {
    // Check for CLI mode
    bool isCli = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--cli" || std::string(argv[i]) == "-c") {
            isCli = true;
        }
    }

    if (isCli) {
        std::cout << "=================================================\n";
        std::cout << "  RW-BENCH 2026 EDITION - Storage Speed Benchmark\n";
        std::cout << "  MilkyCloud Technologies - Direct I/O Engine\n";
        std::cout << "=================================================\n\n";

        std::cout << "Detecting storage drives...\n";
        auto drives = DetectAllDrives();
        for (size_t i = 0; i < drives.size(); ++i) {
            const auto& d = drives[i];
            std::wcout << L"[" << i << L"] " << d.path << L" (FS: " << d.fileSystem 
                       << L", Type: " << d.driveType << L") - "
                       << (d.freeBytes / (1024 * 1024 * 1024)) << L" GB free of "
                       << (d.totalBytes / (1024 * 1024 * 1024)) << L" GB\n";
        }
        return 0;
    }

    BenchmarkEngine engine;
    HttpServer server(engine);

    if (!server.Start(0)) {
        MessageBoxW(nullptr, L"Failed to initialize local network server.", L"RW-Bench Error", MB_ICONERROR);
        return 1;
    }

    uint16_t port = server.GetPort();
    std::wstring url = L"http://127.0.0.1:" + std::to_wstring(port);

    std::wstring browserExe = FindBrowserExecutable();
    HANDLE hBrowserProcess = nullptr;

    if (!browserExe.empty()) {
        wchar_t tempDir[MAX_PATH] = {0};
        GetTempPathW(MAX_PATH, tempDir);
        std::wstring profileDir = std::wstring(tempDir) + L"rwbench_webview_profile";

        std::wstring cmd = L"\"" + browserExe + L"\" --app=" + url +
                           L" --window-size=1260,860 --user-data-dir=\"" + profileDir + L"\"";

        STARTUPINFOW si = {sizeof(si)};
        PROCESS_INFORMATION pi = {0};
        if (CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
            hBrowserProcess = pi.hProcess;
            CloseHandle(pi.hThread);
        }
    }

    // Fallback to default system browser if app mode process failed to launch
    if (!hBrowserProcess) {
        ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }

    // Keep running until browser process terminates or shutdown is signaled
    while (server.IsRunning() && !server.ShouldShutdown()) {
        if (hBrowserProcess) {
            DWORD exitCode = 0;
            if (GetExitCodeProcess(hBrowserProcess, &exitCode) && exitCode != STILL_ACTIVE) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    if (hBrowserProcess) {
        CloseHandle(hBrowserProcess);
    }

    server.Stop();
    engine.Abort();

    return 0;
}
