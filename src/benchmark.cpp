#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "benchmark.hpp"
#include "drives.hpp"
#include <windows.h>
#include <random>
#include <algorithm>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>

static double GetQpcTimeSec() {
    static LARGE_INTEGER freq = {0};
    if (freq.QuadPart == 0) {
        QueryPerformanceFrequency(&freq);
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return static_cast<double>(counter.QuadPart) / static_cast<double>(freq.QuadPart);
}

BenchmarkEngine::BenchmarkEngine() = default;

BenchmarkEngine::~BenchmarkEngine() {
    Abort();
}

bool BenchmarkEngine::Start(const std::wstring& drivePath, TestMode mode, uint64_t targetSizeBytes, uint32_t blockSizeBytes) {
    if (m_isRunning.load()) return false;

    m_shouldStop.store(false);
    m_isRunning.store(true);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_status = LiveStatus{};
        m_status.isRunning = true;
        m_status.phase = "Starting";
        m_status.drivePath = std::string(drivePath.begin(), drivePath.end());
        m_status.totalBytesTarget = targetSizeBytes;
    }

    std::thread([this, drivePath, mode, targetSizeBytes, blockSizeBytes]() {
        RunWorker(drivePath, mode, targetSizeBytes, blockSizeBytes);
    }).detach();

    return true;
}

void BenchmarkEngine::Abort() {
    m_shouldStop.store(true);
    while (m_isRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

LiveStatus BenchmarkEngine::GetStatus() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_status;
}

std::string BenchmarkEngine::GetStatusJson() {
    LiveStatus s = GetStatus();
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"isRunning\": " << (s.isRunning ? "true" : "false") << ",\n";
    ss << "  \"phase\": \"" << s.phase << "\",\n";
    ss << "  \"activeTest\": \"" << s.activeTest << "\",\n";
    ss << "  \"drivePath\": \"" << s.drivePath << "\",\n";
    ss << "  \"fileSystem\": \"" << s.fileSystem << "\",\n";
    ss << "  \"progress\": " << std::fixed << std::setprecision(1) << s.progress << ",\n";
    ss << "  \"instantMBps\": " << std::fixed << std::setprecision(2) << s.instantMBps << ",\n";
    ss << "  \"averageMBps\": " << std::fixed << std::setprecision(2) << s.averageMBps << ",\n";
    ss << "  \"peakMBps\": " << std::fixed << std::setprecision(2) << s.peakMBps << ",\n";
    ss << "  \"currentIOPS\": " << s.currentIOPS << ",\n";
    ss << "  \"bytesProcessed\": " << s.bytesProcessed << ",\n";
    ss << "  \"totalBytesTarget\": " << s.totalBytesTarget << ",\n";
    ss << "  \"elapsedSeconds\": " << std::fixed << std::setprecision(2) << s.elapsedSeconds << ",\n";
    ss << "  \"errorMessage\": \"" << s.errorMessage << "\",\n";

    // Results array
    ss << "  \"completedResults\": [\n";
    for (size_t i = 0; i < s.completedResults.size(); ++i) {
        const auto& r = s.completedResults[i];
        ss << "    {\n";
        ss << "      \"testName\": \"" << r.testName << "\",\n";
        ss << "      \"durationSec\": " << std::fixed << std::setprecision(2) << r.durationSec << ",\n";
        ss << "      \"totalBytes\": " << r.totalBytes << ",\n";
        ss << "      \"avgMBps\": " << std::fixed << std::setprecision(2) << r.avgMBps << ",\n";
        ss << "      \"peakMBps\": " << std::fixed << std::setprecision(2) << r.peakMBps << ",\n";
        ss << "      \"avgIOPS\": " << r.avgIOPS << ",\n";
        ss << "      \"avgLatencyMs\": " << std::fixed << std::setprecision(3) << r.avgLatencyMs << "\n";
        ss << "    }" << (i + 1 < s.completedResults.size() ? ",\n" : "\n");
    }
    ss << "  ],\n";

    // Recent history samples for live chart (last 50 samples)
    ss << "  \"history\": [\n";
    size_t startIdx = (s.history.size() > 50) ? (s.history.size() - 50) : 0;
    for (size_t i = startIdx; i < s.history.size(); ++i) {
        const auto& smp = s.history[i];
        ss << "    {\"t\":" << std::fixed << std::setprecision(2) << smp.timestamp
           << ",\"inst\":" << std::fixed << std::setprecision(2) << smp.instantMBps
           << ",\"avg\":" << std::fixed << std::setprecision(2) << smp.averageMBps
           << ",\"iops\":" << smp.iops << "}"
           << (i + 1 < s.history.size() ? ",\n" : "\n");
    }
    ss << "  ]\n";
    ss << "}";
    return ss.str();
}

void BenchmarkEngine::RunWorker(std::wstring drivePath, TestMode mode, uint64_t targetSizeBytes, uint32_t blockSizeBytes) {
    // Sector size detection
    uint32_t sectorSize = 4096;
    DWORD sectorsPerCluster = 0, bytesPerSector = 0, freeClusters = 0, totalClusters = 0;
    if (GetDiskFreeSpaceW(drivePath.c_str(), &sectorsPerCluster, &bytesPerSector, &freeClusters, &totalClusters)) {
        if (bytesPerSector > 0) sectorSize = bytesPerSector;
    }

    // Align targetSizeBytes to sectorSize
    if (targetSizeBytes % sectorSize != 0) {
        targetSizeBytes = ((targetSizeBytes / sectorSize) + 1) * sectorSize;
    }
    if (blockSizeBytes % sectorSize != 0) {
        blockSizeBytes = ((blockSizeBytes / sectorSize) + 1) * sectorSize;
    }

    // Build temporary test file path in the root or accessible directory of target drive
    std::wstring tempFilePath = drivePath;
    if (tempFilePath.back() != L'\\') tempFilePath += L'\\';
    tempFilePath += L".rwbench_temp_benchmark.dat";

    bool success = true;

    if (mode == TestMode::SequentialWrite || mode == TestMode::FullSuite) {
        BenchmarkResult res;
        if (!RunSequentialWrite(tempFilePath, targetSizeBytes, blockSizeBytes, sectorSize, res)) {
            success = false;
        } else {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_status.completedResults.push_back(res);
        }
    }

    if (success && !m_shouldStop.load() && (mode == TestMode::SequentialRead || mode == TestMode::FullSuite)) {
        BenchmarkResult res;
        if (!RunSequentialRead(tempFilePath, targetSizeBytes, blockSizeBytes, sectorSize, res)) {
            success = false;
        } else {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_status.completedResults.push_back(res);
        }
    }

    if (success && !m_shouldStop.load() && (mode == TestMode::Random4KWrite || mode == TestMode::FullSuite)) {
        BenchmarkResult res;
        if (!RunRandom4KWrite(tempFilePath, targetSizeBytes, 5, sectorSize, res)) {
            success = false;
        } else {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_status.completedResults.push_back(res);
        }
    }

    if (success && !m_shouldStop.load() && (mode == TestMode::Random4KRead || mode == TestMode::FullSuite)) {
        BenchmarkResult res;
        if (!RunRandom4KRead(tempFilePath, targetSizeBytes, 5, sectorSize, res)) {
            success = false;
        } else {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_status.completedResults.push_back(res);
        }
    }

    // Always delete temporary test file
    DeleteFileW(tempFilePath.c_str());

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_status.isRunning = false;
        if (m_shouldStop.load()) {
            m_status.phase = "Aborted";
        } else if (success) {
            m_status.phase = "Completed";
            m_status.progress = 100.0f;
        } else {
            m_status.phase = "Error";
        }
    }

    m_isRunning.store(false);
}

bool BenchmarkEngine::RunSequentialWrite(const std::wstring& filePath, uint64_t targetSize, uint32_t blockSize, uint32_t sectorSize, BenchmarkResult& outResult) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_status.phase = "Running";
        m_status.activeTest = "Sequential Write";
        m_status.bytesProcessed = 0;
        m_status.totalBytesTarget = targetSize;
        m_status.progress = 0.0f;
        m_status.history.clear();
    }

    // Allocate sector-aligned buffer using VirtualAlloc
    void* buffer = VirtualAlloc(nullptr, blockSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!buffer) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_status.errorMessage = "Failed to allocate sector-aligned buffer";
        return false;
    }

    // Fill buffer with pseudo-random pattern to prevent compression bypass
    uint32_t* pWords = static_cast<uint32_t*>(buffer);
    for (size_t i = 0; i < blockSize / sizeof(uint32_t); ++i) {
        pWords[i] = static_cast<uint32_t>(0xA5A5A5A5 ^ i);
    }

    HANDLE hFile = CreateFileW(
        filePath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        VirtualFree(buffer, 0, MEM_RELEASE);
        std::lock_guard<std::mutex> lock(m_mutex);
        m_status.errorMessage = "Failed to create Direct I/O file. Try running as Administrator.";
        return false;
    }

    uint64_t bytesWrittenTotal = 0;
    double startTime = GetQpcTimeSec();
    double lastSampleTime = startTime;
    uint64_t lastSampleBytes = 0;
    double peakMBps = 0.0;
    uint32_t ioCount = 0;

    while (bytesWrittenTotal < targetSize && !m_shouldStop.load()) {
        uint64_t remaining = targetSize - bytesWrittenTotal;
        uint64_t chunk = (static_cast<uint64_t>(blockSize) < remaining) ? static_cast<uint64_t>(blockSize) : remaining;
        DWORD toWrite = static_cast<DWORD>(chunk);
        // Ensure toWrite is sector aligned
        if (toWrite % sectorSize != 0) {
            toWrite = ((toWrite / sectorSize) + 1) * sectorSize;
        }

        DWORD written = 0;
        BOOL ok = WriteFile(hFile, buffer, toWrite, &written, nullptr);
        if (!ok || written == 0) {
            break;
        }

        bytesWrittenTotal += written;
        ioCount++;

        double now = GetQpcTimeSec();
        double deltaWindow = now - lastSampleTime;

        if (deltaWindow >= 0.08) { // update 12 times per second
            double windowBytes = static_cast<double>(bytesWrittenTotal - lastSampleBytes);
            double instMBps = (windowBytes / (1024.0 * 1024.0)) / deltaWindow;
            double elapsed = now - startTime;
            double avgMBps = (static_cast<double>(bytesWrittenTotal) / (1024.0 * 1024.0)) / (elapsed > 0 ? elapsed : 0.001);
            if (instMBps > peakMBps) peakMBps = instMBps;

            uint32_t curIOPS = static_cast<uint32_t>(1.0 / (deltaWindow / 1.0));

            std::lock_guard<std::mutex> lock(m_mutex);
            m_status.instantMBps = instMBps;
            m_status.averageMBps = avgMBps;
            m_status.peakMBps = peakMBps;
            m_status.currentIOPS = curIOPS;
            m_status.bytesProcessed = bytesWrittenTotal;
            m_status.elapsedSeconds = elapsed;
            float pct = static_cast<float>((bytesWrittenTotal * 100.0) / targetSize);
            m_status.progress = (pct > 100.0f) ? 100.0f : pct;
            m_status.history.push_back({elapsed, instMBps, avgMBps, curIOPS});

            lastSampleTime = now;
            lastSampleBytes = bytesWrittenTotal;
        }
    }

    CloseHandle(hFile);
    VirtualFree(buffer, 0, MEM_RELEASE);

    double totalTime = GetQpcTimeSec() - startTime;
    double finalAvg = (static_cast<double>(bytesWrittenTotal) / (1024.0 * 1024.0)) / (totalTime > 0 ? totalTime : 0.001);

    outResult.testName = "Sequential Write";
    outResult.durationSec = totalTime;
    outResult.totalBytes = bytesWrittenTotal;
    outResult.avgMBps = finalAvg;
    outResult.peakMBps = peakMBps;
    outResult.avgIOPS = (totalTime > 0) ? static_cast<uint32_t>(ioCount / totalTime) : 0;
    outResult.avgLatencyMs = (ioCount > 0) ? (totalTime * 1000.0 / ioCount) : 0.0;

    return !m_shouldStop.load();
}

bool BenchmarkEngine::RunSequentialRead(const std::wstring& filePath, uint64_t targetSize, uint32_t blockSize, uint32_t sectorSize, BenchmarkResult& outResult) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_status.phase = "Running";
        m_status.activeTest = "Sequential Read";
        m_status.bytesProcessed = 0;
        m_status.totalBytesTarget = targetSize;
        m_status.progress = 0.0f;
        m_status.history.clear();
    }

    void* buffer = VirtualAlloc(nullptr, blockSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!buffer) return false;

    HANDLE hFile = CreateFileW(
        filePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        VirtualFree(buffer, 0, MEM_RELEASE);
        return false;
    }

    uint64_t bytesReadTotal = 0;
    double startTime = GetQpcTimeSec();
    double lastSampleTime = startTime;
    uint64_t lastSampleBytes = 0;
    double peakMBps = 0.0;
    uint32_t ioCount = 0;

    while (bytesReadTotal < targetSize && !m_shouldStop.load()) {
        uint64_t remaining = targetSize - bytesReadTotal;
        uint64_t chunk = (static_cast<uint64_t>(blockSize) < remaining) ? static_cast<uint64_t>(blockSize) : remaining;
        DWORD toRead = static_cast<DWORD>(chunk);
        if (toRead % sectorSize != 0) {
            toRead = ((toRead / sectorSize) + 1) * sectorSize;
        }

        DWORD readBytes = 0;
        BOOL ok = ReadFile(hFile, buffer, toRead, &readBytes, nullptr);
        if (!ok || readBytes == 0) break;

        bytesReadTotal += readBytes;
        ioCount++;

        double now = GetQpcTimeSec();
        double deltaWindow = now - lastSampleTime;

        if (deltaWindow >= 0.08) {
            double windowBytes = static_cast<double>(bytesReadTotal - lastSampleBytes);
            double instMBps = (windowBytes / (1024.0 * 1024.0)) / deltaWindow;
            double elapsed = now - startTime;
            double avgMBps = (static_cast<double>(bytesReadTotal) / (1024.0 * 1024.0)) / (elapsed > 0 ? elapsed : 0.001);
            if (instMBps > peakMBps) peakMBps = instMBps;

            uint32_t curIOPS = static_cast<uint32_t>(1.0 / (deltaWindow / 1.0));

            std::lock_guard<std::mutex> lock(m_mutex);
            m_status.instantMBps = instMBps;
            m_status.averageMBps = avgMBps;
            m_status.peakMBps = peakMBps;
            m_status.currentIOPS = curIOPS;
            m_status.bytesProcessed = bytesReadTotal;
            m_status.elapsedSeconds = elapsed;
            float pct = static_cast<float>((bytesReadTotal * 100.0) / targetSize);
            m_status.progress = (pct > 100.0f) ? 100.0f : pct;
            m_status.history.push_back({elapsed, instMBps, avgMBps, curIOPS});

            lastSampleTime = now;
            lastSampleBytes = bytesReadTotal;
        }
    }

    CloseHandle(hFile);
    VirtualFree(buffer, 0, MEM_RELEASE);

    double totalTime = GetQpcTimeSec() - startTime;
    double finalAvg = (static_cast<double>(bytesReadTotal) / (1024.0 * 1024.0)) / (totalTime > 0 ? totalTime : 0.001);

    outResult.testName = "Sequential Read";
    outResult.durationSec = totalTime;
    outResult.totalBytes = bytesReadTotal;
    outResult.avgMBps = finalAvg;
    outResult.peakMBps = peakMBps;
    outResult.avgIOPS = (totalTime > 0) ? static_cast<uint32_t>(ioCount / totalTime) : 0;
    outResult.avgLatencyMs = (ioCount > 0) ? (totalTime * 1000.0 / ioCount) : 0.0;

    return !m_shouldStop.load();
}

bool BenchmarkEngine::RunRandom4KWrite(const std::wstring& filePath, uint64_t targetSize, uint32_t durationSeconds, uint32_t sectorSize, BenchmarkResult& outResult) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_status.phase = "Running";
        m_status.activeTest = "Random 4K Write (IOPS)";
        m_status.bytesProcessed = 0;
        m_status.totalBytesTarget = targetSize;
        m_status.progress = 0.0f;
        m_status.history.clear();
    }

    uint32_t blockSize = 4096;
    if (blockSize < sectorSize) blockSize = sectorSize;

    void* buffer = VirtualAlloc(nullptr, blockSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!buffer) return false;

    memset(buffer, 0x5A, blockSize);

    HANDLE hFile = CreateFileW(
        filePath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_ALWAYS,
        FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        VirtualFree(buffer, 0, MEM_RELEASE);
        return false;
    }

    uint64_t maxBlocks = (targetSize / blockSize);
    if (maxBlocks == 0) maxBlocks = 1;

    std::mt19937_64 rng(1337);
    std::uniform_int_distribution<uint64_t> dist(0, maxBlocks - 1);

    double startTime = GetQpcTimeSec();
    double lastSampleTime = startTime;
    uint32_t ioCount = 0;
    uint32_t lastSampleIO = 0;
    uint64_t totalBytesWritten = 0;
    double peakMBps = 0.0;

    while (!m_shouldStop.load()) {
        double now = GetQpcTimeSec();
        double elapsed = now - startTime;
        if (elapsed >= durationSeconds) break;

        uint64_t blockIdx = dist(rng);
        LARGE_INTEGER offset;
        offset.QuadPart = blockIdx * blockSize;

        OVERLAPPED ov = {0};
        ov.Offset = offset.LowPart;
        ov.OffsetHigh = offset.HighPart;

        DWORD written = 0;
        if (WriteFile(hFile, buffer, blockSize, &written, &ov)) {
            totalBytesWritten += written;
            ioCount++;
        }

        double deltaWindow = now - lastSampleTime;
        if (deltaWindow >= 0.1) {
            uint32_t windowIO = ioCount - lastSampleIO;
            uint32_t instIOPS = static_cast<uint32_t>(windowIO / deltaWindow);
            double instMBps = (static_cast<double>(windowIO * blockSize) / (1024.0 * 1024.0)) / deltaWindow;
            double avgMBps = (static_cast<double>(totalBytesWritten) / (1024.0 * 1024.0)) / (elapsed > 0 ? elapsed : 0.001);
            if (instMBps > peakMBps) peakMBps = instMBps;

            std::lock_guard<std::mutex> lock(m_mutex);
            m_status.instantMBps = instMBps;
            m_status.averageMBps = avgMBps;
            m_status.peakMBps = peakMBps;
            m_status.currentIOPS = instIOPS;
            m_status.bytesProcessed = totalBytesWritten;
            m_status.elapsedSeconds = elapsed;
            float pct = static_cast<float>((elapsed * 100.0) / durationSeconds);
            m_status.progress = (pct > 100.0f) ? 100.0f : pct;
            m_status.history.push_back({elapsed, instMBps, avgMBps, instIOPS});

            lastSampleTime = now;
            lastSampleIO = ioCount;
        }
    }

    CloseHandle(hFile);
    VirtualFree(buffer, 0, MEM_RELEASE);

    double totalTime = GetQpcTimeSec() - startTime;
    outResult.testName = "Random 4K Write";
    outResult.durationSec = totalTime;
    outResult.totalBytes = totalBytesWritten;
    outResult.avgMBps = (totalTime > 0) ? (static_cast<double>(totalBytesWritten) / (1024.0 * 1024.0)) / totalTime : 0.0;
    outResult.peakMBps = peakMBps;
    outResult.avgIOPS = (totalTime > 0) ? static_cast<uint32_t>(ioCount / totalTime) : 0;
    outResult.avgLatencyMs = (ioCount > 0) ? (totalTime * 1000.0 / ioCount) : 0.0;

    return !m_shouldStop.load();
}

bool BenchmarkEngine::RunRandom4KRead(const std::wstring& filePath, uint64_t targetSize, uint32_t durationSeconds, uint32_t sectorSize, BenchmarkResult& outResult) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_status.phase = "Running";
        m_status.activeTest = "Random 4K Read (IOPS)";
        m_status.bytesProcessed = 0;
        m_status.totalBytesTarget = targetSize;
        m_status.progress = 0.0f;
        m_status.history.clear();
    }

    uint32_t blockSize = 4096;
    if (blockSize < sectorSize) blockSize = sectorSize;

    void* buffer = VirtualAlloc(nullptr, blockSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!buffer) return false;

    HANDLE hFile = CreateFileW(
        filePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        VirtualFree(buffer, 0, MEM_RELEASE);
        return false;
    }

    uint64_t maxBlocks = (targetSize / blockSize);
    if (maxBlocks == 0) maxBlocks = 1;

    std::mt19937_64 rng(4242);
    std::uniform_int_distribution<uint64_t> dist(0, maxBlocks - 1);

    double startTime = GetQpcTimeSec();
    double lastSampleTime = startTime;
    uint32_t ioCount = 0;
    uint32_t lastSampleIO = 0;
    uint64_t totalBytesRead = 0;
    double peakMBps = 0.0;

    while (!m_shouldStop.load()) {
        double now = GetQpcTimeSec();
        double elapsed = now - startTime;
        if (elapsed >= durationSeconds) break;

        uint64_t blockIdx = dist(rng);
        LARGE_INTEGER offset;
        offset.QuadPart = blockIdx * blockSize;

        OVERLAPPED ov = {0};
        ov.Offset = offset.LowPart;
        ov.OffsetHigh = offset.HighPart;

        DWORD readBytes = 0;
        if (ReadFile(hFile, buffer, blockSize, &readBytes, &ov)) {
            totalBytesRead += readBytes;
            ioCount++;
        }

        double deltaWindow = now - lastSampleTime;
        if (deltaWindow >= 0.1) {
            uint32_t windowIO = ioCount - lastSampleIO;
            uint32_t instIOPS = static_cast<uint32_t>(windowIO / deltaWindow);
            double instMBps = (static_cast<double>(windowIO * blockSize) / (1024.0 * 1024.0)) / deltaWindow;
            double avgMBps = (static_cast<double>(totalBytesRead) / (1024.0 * 1024.0)) / (elapsed > 0 ? elapsed : 0.001);
            if (instMBps > peakMBps) peakMBps = instMBps;

            std::lock_guard<std::mutex> lock(m_mutex);
            m_status.instantMBps = instMBps;
            m_status.averageMBps = avgMBps;
            m_status.peakMBps = peakMBps;
            m_status.currentIOPS = instIOPS;
            m_status.bytesProcessed = totalBytesRead;
            m_status.elapsedSeconds = elapsed;
            float pct = static_cast<float>((elapsed * 100.0) / durationSeconds);
            m_status.progress = (pct > 100.0f) ? 100.0f : pct;
            m_status.history.push_back({elapsed, instMBps, avgMBps, instIOPS});

            lastSampleTime = now;
            lastSampleIO = ioCount;
        }
    }

    CloseHandle(hFile);
    VirtualFree(buffer, 0, MEM_RELEASE);

    double totalTime = GetQpcTimeSec() - startTime;
    outResult.testName = "Random 4K Read";
    outResult.durationSec = totalTime;
    outResult.totalBytes = totalBytesRead;
    outResult.avgMBps = (totalTime > 0) ? (static_cast<double>(totalBytesRead) / (1024.0 * 1024.0)) / totalTime : 0.0;
    outResult.peakMBps = peakMBps;
    outResult.avgIOPS = (totalTime > 0) ? static_cast<uint32_t>(ioCount / totalTime) : 0;
    outResult.avgLatencyMs = (ioCount > 0) ? (totalTime * 1000.0 / ioCount) : 0.0;

    return !m_shouldStop.load();
}
