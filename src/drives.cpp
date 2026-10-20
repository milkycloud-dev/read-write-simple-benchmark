#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "drives.hpp"
#include <windows.h>
#include <sstream>
#include <iomanip>

static std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    if (size <= 0) return "";
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &result[0], size, nullptr, nullptr);
    return result;
}

static std::string EscapeJsonString(const std::string& input) {
    std::ostringstream ss;
    for (char c : input) {
        switch (c) {
            case '"': ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                } else {
                    ss << c;
                }
        }
    }
    return ss.str();
}

std::vector<DriveInfo> DetectAllDrives() {
    std::vector<DriveInfo> list;
    DWORD bufferLen = GetLogicalDriveStringsW(0, nullptr);
    if (bufferLen == 0) return list;

    std::vector<wchar_t> buffer(bufferLen + 1);
    if (!GetLogicalDriveStringsW(bufferLen, buffer.data())) return list;

    const wchar_t* pDrive = buffer.data();
    while (*pDrive) {
        DriveInfo info;
        info.path = pDrive; // e.g. "C:\\"

        UINT type = GetDriveTypeW(pDrive);
        switch (type) {
            case DRIVE_FIXED:     info.driveType = L"Fixed SSD/HDD"; break;
            case DRIVE_REMOVABLE: info.driveType = L"Removable USB"; break;
            case DRIVE_REMOTE:    info.driveType = L"Network Drive"; break;
            case DRIVE_CDROM:     info.driveType = L"Optical Drive"; break;
            case DRIVE_RAMDISK:   info.driveType = L"RAM Disk"; break;
            default:              info.driveType = L"Other Storage"; break;
        }

        // Only test drives that are writable storages (Fixed, Removable, RAM Disk)
        if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE || type == DRIVE_RAMDISK) {
            wchar_t volumeName[MAX_PATH + 1] = {0};
            wchar_t fsName[MAX_PATH + 1] = {0};
            DWORD serialNumber = 0, maxComponentLen = 0, flags = 0;

            // Suppress Windows error dialog (e.g. if removable drive has no media)
            UINT oldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

            if (GetVolumeInformationW(pDrive, volumeName, MAX_PATH, &serialNumber, &maxComponentLen, &flags, fsName, MAX_PATH)) {
                info.volumeName = volumeName;
                info.fileSystem = fsName;
            } else {
                info.fileSystem = L"Unknown";
            }

            ULARGE_INTEGER freeBytesCaller, totalBytes, totalFreeBytes;
            if (GetDiskFreeSpaceExW(pDrive, &freeBytesCaller, &totalBytes, &totalFreeBytes)) {
                info.totalBytes = totalBytes.QuadPart;
                info.freeBytes = freeBytesCaller.QuadPart;
            }

            DWORD sectorsPerCluster = 0, bytesPerSector = 0, freeClusters = 0, totalClusters = 0;
            if (GetDiskFreeSpaceW(pDrive, &sectorsPerCluster, &bytesPerSector, &freeClusters, &totalClusters)) {
                info.sectorSize = (bytesPerSector > 0) ? bytesPerSector : 4096;
            } else {
                info.sectorSize = 4096;
            }

            SetErrorMode(oldErrorMode);

            info.isWritable = (info.freeBytes > 1024 * 1024 * 64); // at least 64MB free
            list.push_back(info);
        }

        pDrive += wcslen(pDrive) + 1;
    }

    return list;
}

std::string DrivesToJson(const std::vector<DriveInfo>& drives) {
    std::ostringstream ss;
    ss << "[\n";
    for (size_t i = 0; i < drives.size(); ++i) {
        const auto& d = drives[i];
        ss << "  {\n";
        ss << "    \"path\": \"" << EscapeJsonString(WideToUtf8(d.path)) << "\",\n";
        ss << "    \"volumeName\": \"" << EscapeJsonString(WideToUtf8(d.volumeName)) << "\",\n";
        ss << "    \"fileSystem\": \"" << EscapeJsonString(WideToUtf8(d.fileSystem)) << "\",\n";
        ss << "    \"driveType\": \"" << EscapeJsonString(WideToUtf8(d.driveType)) << "\",\n";
        ss << "    \"totalBytes\": " << d.totalBytes << ",\n";
        ss << "    \"freeBytes\": " << d.freeBytes << ",\n";
        ss << "    \"sectorSize\": " << d.sectorSize << ",\n";
        ss << "    \"isWritable\": " << (d.isWritable ? "true" : "false") << "\n";
        ss << "  }" << (i + 1 < drives.size() ? ",\n" : "\n");
    }
    ss << "]";
    return ss.str();
}
