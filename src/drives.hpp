#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct DriveInfo {
    std::wstring path;          // e.g. L"C:\\"
    std::wstring volumeName;    // e.g. L"System"
    std::wstring fileSystem;    // e.g. L"NTFS", L"FAT32", L"exFAT", L"ReFS"
    std::wstring driveType;     // e.g. L"Fixed SSD/HDD", L"Removable USB"
    uint64_t totalBytes = 0;
    uint64_t freeBytes = 0;
    uint32_t sectorSize = 4096;
    bool isWritable = true;
};

std::vector<DriveInfo> DetectAllDrives();
std::string DrivesToJson(const std::vector<DriveInfo>& drives);
