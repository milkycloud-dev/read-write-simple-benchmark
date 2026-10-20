#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <cstdint>

enum class TestMode {
    SequentialWrite,
    SequentialRead,
    Random4KWrite,
    Random4KRead,
    FullSuite
};

struct BenchmarkSample {
    double timestamp;      // seconds since test start
    double instantMBps;    // instantaneous speed
    double averageMBps;    // overall average speed
    uint32_t iops;         // current IOPS
};

struct BenchmarkResult {
    std::string testName;
    double durationSec = 0.0;
    uint64_t totalBytes = 0;
    double avgMBps = 0.0;
    double peakMBps = 0.0;
    uint32_t avgIOPS = 0;
    double minLatencyMs = 0.0;
    double avgLatencyMs = 0.0;
    double maxLatencyMs = 0.0;
};

struct LiveStatus {
    bool isRunning = false;
    std::string phase = "Idle";
    std::string activeTest = "";
    std::string drivePath = "";
    std::string fileSystem = "";
    float progress = 0.0f;           // 0.0 - 100.0
    double instantMBps = 0.0;
    double averageMBps = 0.0;
    double peakMBps = 0.0;
    uint32_t currentIOPS = 0;
    uint64_t bytesProcessed = 0;
    uint64_t totalBytesTarget = 0;
    double elapsedSeconds = 0.0;
    std::string errorMessage = "";
    std::vector<BenchmarkSample> history;
    std::vector<BenchmarkResult> completedResults;
};

class BenchmarkEngine {
public:
    BenchmarkEngine();
    ~BenchmarkEngine();

    bool Start(const std::wstring& drivePath, TestMode mode, uint64_t targetSizeBytes, uint32_t blockSizeBytes);
    void Abort();
    LiveStatus GetStatus();
    std::string GetStatusJson();

private:
    void RunWorker(std::wstring drivePath, TestMode mode, uint64_t targetSizeBytes, uint32_t blockSizeBytes);
    
    bool RunSequentialWrite(const std::wstring& filePath, uint64_t targetSize, uint32_t blockSize, uint32_t sectorSize, BenchmarkResult& outResult);
    bool RunSequentialRead(const std::wstring& filePath, uint64_t targetSize, uint32_t blockSize, uint32_t sectorSize, BenchmarkResult& outResult);
    bool RunRandom4KWrite(const std::wstring& filePath, uint64_t targetSize, uint32_t durationSeconds, uint32_t sectorSize, BenchmarkResult& outResult);
    bool RunRandom4KRead(const std::wstring& filePath, uint64_t targetSize, uint32_t durationSeconds, uint32_t sectorSize, BenchmarkResult& outResult);

    std::atomic<bool> m_isRunning{false};
    std::atomic<bool> m_shouldStop{false};
    std::mutex m_mutex;
    LiveStatus m_status;
};
