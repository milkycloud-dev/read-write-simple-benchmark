# RW-Bench

[![CI Build & Release](https://github.com/milkycloud-dev/read-write-simple-benchmark/actions/workflows/build-and-release.yml/badge.svg)](https://github.com/milkycloud-dev/read-write-simple-benchmark/actions/workflows/build-and-release.yml)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-blue.svg)](https://github.com/milkycloud-dev/read-write-simple-benchmark)
[![Language](https://img.shields.io/badge/C%2B%2B-20-purple.svg)](https://github.com/milkycloud-dev/read-write-simple-benchmark)
[![GUI](https://img.shields.io/badge/GUI-Native%20Win32%2FGDI%2B-green.svg)](https://github.com/milkycloud-dev/read-write-simple-benchmark)
[![License](https://img.shields.io/badge/License-Proprietary-red.svg)](LICENSE)

A simple disk read/write speed benchmark utility for Windows written in C++ with a native Win32/GDI+ dark interface.

---

## Highlights

- **Pure Native Windows Application**:
  - Operates inside its own dedicated Win32 desktop window with Windows 11/10 immersive dark mode.
  - Zero browser dependencies (no Chrome, Edge, or webviews).
  - Razor-sharp native typography utilizing Microsoft **Segoe UI** with ClearType subpixel rendering.
  - Zero-flicker double-buffered **60 FPS** GDI+ hardware-timed rendering.
  - Ultra-lightweight footprint: starts in <15 ms and uses <15 MB of RAM.

- **Direct I/O Hardware Measurement**:
  - Utilizes low-level Win32 `FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH` with sector-aligned buffers (`VirtualAlloc`).
  - Bypasses the Windows file system cache to measure true physical controller and drive performance.

- **Universal Storage & Volume Detection**:
  - Automatically scans and lists all logical drives from `A:\` to `Z:\`:
    - **Fixed Disks** (NVMe, SATA SSD, HDD)
    - **Removable Storages** (USB Flash Drives, External SSDs, SD Cards)
    - **RAM Disks & Virtual Volumes**
    - **Network Shares**
  - Supports all Windows file systems: **NTFS**, **FAT32**, **exFAT**, **ReFS**.
  - Includes a **Custom Path / Folder** picker to benchmark any specific directory or mount point.

- **Clean & Focused Dashboard**:
  - Large digital speed readout with instantaneous throughput, peak speed, average speed, and IOPS.
  - Real-time animated throughput wave chart.
  - Compact summary table covering Sequential Write, Sequential Read, and Random 4K operations.

---

## Building from Source

All builds are automated through GitHub Actions CI/CD to prevent consuming local system resources.

### Manual CMake Build (Windows)
```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Release -A x64
cmake --build build --config Release
```
The resulting standalone executable will be located in `build/Release/rw-benchmark.exe`.

---

## Command Line Usage

To inspect detected volumes headlessly:
```powershell
rw-benchmark.exe --cli
```

---

## License

Copyright (c) 2026 milkycloud-dev. All rights reserved.  
Distributed under the **MilkyCloud Proprietary License**. See [LICENSE](LICENSE) for full legal terms.
