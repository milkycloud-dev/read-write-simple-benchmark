#include "server.hpp"
#include "benchmark.hpp"
#include "drives.hpp"
#include "embedded_assets.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <thread>
#include <sstream>
#include <iostream>
#include <vector>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

static std::string ExtractJsonField(const std::string& json, const std::string& field) {
    std::string key = "\"" + field + "\"";
    size_t pos = json.find(key);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    if (pos >= json.size()) return "";

    if (json[pos] == '"') {
        size_t endPos = json.find('"', pos + 1);
        if (endPos == std::string::npos) return "";
        return json.substr(pos + 1, endPos - pos - 1);
    } else {
        size_t endPos = json.find_first_of(",}\r\n", pos);
        if (endPos == std::string::npos) endPos = json.size();
        return json.substr(pos, endPos - pos);
    }
}

HttpServer::HttpServer(BenchmarkEngine& engine)
    : m_engine(engine) {}

HttpServer::~HttpServer() {
    Stop();
}

bool HttpServer::Start(uint16_t requestedPort) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }

    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1
    addr.sin_port = htons(requestedPort);

    if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        closesocket(s);
        WSACleanup();
        return false;
    }

    sockaddr_in boundAddr = {0};
    int boundLen = sizeof(boundAddr);
    if (getsockname(s, reinterpret_cast<sockaddr*>(&boundAddr), &boundLen) == 0) {
        m_port = ntohs(boundAddr.sin_port);
    } else {
        m_port = requestedPort;
    }

    if (listen(s, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(s);
        WSACleanup();
        return false;
    }

    m_listenSocket = static_cast<uintptr_t>(s);
    m_running.store(true);

    std::thread(&HttpServer::ListenerThread, this).detach();
    return true;
}

void HttpServer::Stop() {
    if (!m_running.load()) return;
    m_running.store(false);

    if (m_listenSocket != 0) {
        closesocket(static_cast<SOCKET>(m_listenSocket));
        m_listenSocket = 0;
    }
    WSACleanup();
}

void HttpServer::ListenerThread() {
    while (m_running.load()) {
        SOCKET client = accept(static_cast<SOCKET>(m_listenSocket), nullptr, nullptr);
        if (client == INVALID_SOCKET) {
            break;
        }

        std::thread(&HttpServer::HandleClient, this, static_cast<uintptr_t>(client)).detach();
    }
}

void HttpServer::HandleClient(uintptr_t clientSocket) {
    SOCKET s = static_cast<SOCKET>(clientSocket);
    std::vector<char> buffer(16384, 0);
    int received = recv(s, buffer.data(), static_cast<int>(buffer.size()) - 1, 0);

    if (received <= 0) {
        closesocket(s);
        return;
    }

    std::string request(buffer.data(), received);
    std::istringstream reqStream(request);
    std::string method, path, proto;
    reqStream >> method >> path >> proto;

    // Separate body if present
    std::string body = "";
    size_t headerEnd = request.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        body = request.substr(headerEnd + 4);
    }

    std::ostringstream response;

    if (method == "GET") {
        if (path == "/" || path == "/index.html") {
            response << "HTTP/1.1 200 OK\r\n"
                     << "Content-Type: text/html; charset=utf-8\r\n"
                     << "Content-Length: " << sizeof(EmbeddedAssets::INDEX_HTML) - 1 << "\r\n"
                     << "Connection: close\r\n\r\n"
                     << EmbeddedAssets::INDEX_HTML;
        } else if (path == "/style.css") {
            response << "HTTP/1.1 200 OK\r\n"
                     << "Content-Type: text/css; charset=utf-8\r\n"
                     << "Content-Length: " << sizeof(EmbeddedAssets::STYLE_CSS) - 1 << "\r\n"
                     << "Connection: close\r\n\r\n"
                     << EmbeddedAssets::STYLE_CSS;
        } else if (path == "/app.js") {
            response << "HTTP/1.1 200 OK\r\n"
                     << "Content-Type: application/javascript; charset=utf-8\r\n"
                     << "Content-Length: " << sizeof(EmbeddedAssets::APP_JS) - 1 << "\r\n"
                     << "Connection: close\r\n\r\n"
                     << EmbeddedAssets::APP_JS;
        } else if (path == "/api/drives") {
            auto drives = DetectAllDrives();
            std::string json = DrivesToJson(drives);
            response << "HTTP/1.1 200 OK\r\n"
                     << "Content-Type: application/json; charset=utf-8\r\n"
                     << "Content-Length: " << json.size() << "\r\n"
                     << "Connection: close\r\n\r\n"
                     << json;
        } else if (path == "/api/status") {
            std::string json = m_engine.GetStatusJson();
            response << "HTTP/1.1 200 OK\r\n"
                     << "Content-Type: application/json; charset=utf-8\r\n"
                     << "Content-Length: " << json.size() << "\r\n"
                     << "Connection: close\r\n\r\n"
                     << json;
        } else {
            std::string notFound = "Not Found";
            response << "HTTP/1.1 404 Not Found\r\n"
                     << "Content-Type: text/plain\r\n"
                     << "Content-Length: " << notFound.size() << "\r\n"
                     << "Connection: close\r\n\r\n"
                     << notFound;
        }
    } else if (method == "POST") {
        if (path == "/api/start") {
            std::string drive = ExtractJsonField(body, "drive");
            std::string modeStr = ExtractJsonField(body, "mode");
            std::string sizeStr = ExtractJsonField(body, "size");
            std::string blockStr = ExtractJsonField(body, "blockSize");

            uint64_t targetSize = sizeStr.empty() ? (256 * 1024 * 1024) : std::stoull(sizeStr);
            uint32_t blockSize = blockStr.empty() ? (1024 * 1024) : static_cast<uint32_t>(std::stoul(blockStr));

            TestMode mode = TestMode::FullSuite;
            if (modeStr == "seq-write") mode = TestMode::SequentialWrite;
            else if (modeStr == "seq-read") mode = TestMode::SequentialRead;
            else if (modeStr == "rnd-4k") mode = TestMode::Random4KWrite;

            std::wstring wDrive(drive.begin(), drive.end());
            bool started = m_engine.Start(wDrive, mode, targetSize, blockSize);

            std::string respJson = started ? "{\"success\":true}" : "{\"success\":false,\"error\":\"Already running\"}";
            response << "HTTP/1.1 200 OK\r\n"
                     << "Content-Type: application/json; charset=utf-8\r\n"
                     << "Content-Length: " << respJson.size() << "\r\n"
                     << "Connection: close\r\n\r\n"
                     << respJson;
        } else if (path == "/api/stop") {
            m_engine.Abort();
            std::string respJson = "{\"success\":true}";
            response << "HTTP/1.1 200 OK\r\n"
                     << "Content-Type: application/json; charset=utf-8\r\n"
                     << "Content-Length: " << respJson.size() << "\r\n"
                     << "Connection: close\r\n\r\n"
                     << respJson;
        } else if (path == "/api/exit") {
            m_shutdownRequested.store(true);
            std::string respJson = "{\"success\":true}";
            response << "HTTP/1.1 200 OK\r\n"
                     << "Content-Type: application/json; charset=utf-8\r\n"
                     << "Content-Length: " << respJson.size() << "\r\n"
                     << "Connection: close\r\n\r\n"
                     << respJson;
        } else {
            response << "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";
        }
    } else {
        response << "HTTP/1.1 405 Method Not Allowed\r\nConnection: close\r\n\r\n";
    }

    std::string respStr = response.str();
    send(s, respStr.data(), static_cast<int>(respStr.size()), 0);
    closesocket(s);
}
