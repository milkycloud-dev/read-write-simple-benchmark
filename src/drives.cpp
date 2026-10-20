#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "drives.hpp"
#include <windows.h>
#include <sstream>
#include <iomanip>

std::wstring DriveInfo::GetDisplayName() const {
    std::wostringstream ss;
    ss << path;
    if (!volumeName.empty()) {
        ss << L" [" << volumeName << L"]";
    }
    if (!fileSystem.empty() && fileSystem != L"Unknown") {
        ss << L" (" << fileSystem << L")";
    } else {
        ss << L" (" << driveType << L")";
    }

    if (totalBytes > 0) {
        double freeGb = static_cast<double>(freeBytes) / (1024.0 * 1024.0 * 1024.0);
        double totalGb = static_cast<double>(totalBytes) / (1024.0 * 1024.0 * 1024.0);
        ss << L" - " << std::fixed << std::setprecision(1) << freeGb << L" GB free / " << totalGb << L" GB";
    }

    return ss.str();
}

std::wstring DriveInfo::GetShortName() const {
    std::wostringstream ss;
    ss << path;
    if (!volumeName.empty()) {
        ss << L" [" << volumeName << L"]";
    } else if (!fileSystem.empty() && fileSystem != L"Unknown") {
        ss << L" (" << fileSystem << L")";
    }
    if (totalBytes > 0) {
        double freeGb = static_cast<double>(freeBytes) / (1024.0 * 1024.0 * 1024.0);
        ss << L" \u2022 " << std::fixed << std::setprecision(0) << freeGb << L" GB Free";
    }
    return ss.str();
}

std::wstring DriveInfo::GetVeryShortName() const {
    std::wostringstream ss;
    if (path.length() >= 2) {
        ss << path.substr(0, 2);
    } else {
        ss << path;
    }
    if (!fileSystem.empty() && fileSystem != L"Unknown") {
        ss << L" (" << fileSystem << L")";
    }
    if (totalBytes > 0) {
        double freeGb = static_cast<double>(freeBytes) / (1024.0 * 1024.0 * 1024.0);
        ss << L" " << std::fixed << std::setprecision(0) << freeGb << L"G";
    }
    return ss.str();
}

std::vector<DriveInfo> DetectAllDrives() {
    std::vector<DriveInfo> list;

    // Suppress system error popups (e.g., removable drive without card inserted)
    UINT oldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    // Scan all drive letters from A to Z
    for (wchar_t letter = L'A'; letter <= L'Z'; ++letter) {
        std::wstring driveRoot = std::wstring(1, letter) + L":\\";
        UINT type = GetDriveTypeW(driveRoot.c_str());

        if (type == DRIVE_NO_ROOT_DIR || type == DRIVE_UNKNOWN) {
            continue;
        }

        DriveInfo info;
        info.path = driveRoot;

        switch (type) {
            case DRIVE_FIXED:     info.driveType = L"Fixed Disk"; break;
            case DRIVE_REMOVABLE: info.driveType = L"Removable / USB"; break;
            case DRIVE_REMOTE:    info.driveType = L"Network Share"; break;
            case DRIVE_RAMDISK:   info.driveType = L"RAM Disk"; break;
            case DRIVE_CDROM:     info.driveType = L"CD/DVD-ROM"; break;
            default:              info.driveType = L"Storage Device"; break;
        }

        // Check capacity
        ULARGE_INTEGER freeCaller = {0}, total = {0}, totalFree = {0};
        if (GetDiskFreeSpaceExW(driveRoot.c_str(), &freeCaller, &total, &totalFree)) {
            info.freeBytes = freeCaller.QuadPart;
            info.totalBytes = total.QuadPart;
        }

        // Query Volume Information
        wchar_t volName[MAX_PATH + 1] = {0};
        wchar_t fsName[MAX_PATH + 1] = {0};
        DWORD serial = 0, maxComponent = 0, flags = 0;

        if (GetVolumeInformationW(driveRoot.c_str(), volName, MAX_PATH, &serial, &maxComponent, &flags, fsName, MAX_PATH)) {
            info.volumeName = volName;
            info.fileSystem = fsName;
        } else {
            info.fileSystem = (type == DRIVE_CDROM) ? L"UDF/CDFS" : L"Unknown";
        }

        // Query sector size
        DWORD sectorsPerCluster = 0, bytesPerSector = 0, freeClusters = 0, totalClusters = 0;
        if (GetDiskFreeSpaceW(driveRoot.c_str(), &sectorsPerCluster, &bytesPerSector, &freeClusters, &totalClusters)) {
            info.sectorSize = (bytesPerSector > 0) ? bytesPerSector : 4096;
        } else {
            info.sectorSize = 4096;
        }

        info.isWritable = (type != DRIVE_CDROM && info.freeBytes > 1024 * 1024 * 32);

        // Include any storage with accessible size or fixed/removable/remote status
        if (type != DRIVE_CDROM || info.totalBytes > 0) {
            list.push_back(info);
        }
    }

    SetErrorMode(oldErrorMode);
    return list;
}
