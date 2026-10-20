#pragma once

#include <string>
#include <atomic>
#include <cstdint>

class BenchmarkEngine;

class HttpServer {
public:
    HttpServer(BenchmarkEngine& engine);
    ~HttpServer();

    bool Start(uint16_t requestedPort = 0);
    void Stop();
    uint16_t GetPort() const { return m_port; }
    bool IsRunning() const { return m_running.load(); }
    void TriggerShutdown() { m_shutdownRequested.store(true); }
    bool ShouldShutdown() const { return m_shutdownRequested.load(); }

private:
    void ListenerThread();
    void HandleClient(uintptr_t clientSocket);

    BenchmarkEngine& m_engine;
    uint16_t m_port = 0;
    uintptr_t m_listenSocket = 0;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_shutdownRequested{false};
};
