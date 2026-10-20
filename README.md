# RW-Bench (2026 Edition)

[![CI Build & Release](https://github.com/milkycloud-dev/read-write-simple-benchmark/actions/workflows/build-and-release.yml/badge.svg)](https://github.com/milkycloud-dev/read-write-simple-benchmark/actions/workflows/build-and-release.yml)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-blue.svg)](https://github.com/milkycloud-dev/read-write-simple-benchmark)
[![Language](https://img.shields.io/badge/C%2B%2B-20-purple.svg)](https://github.com/milkycloud-dev/read-write-simple-benchmark)
[![License](https://img.shields.io/badge/License-Proprietary-red.svg)](LICENSE)

A high-performance live disk read/write throughput benchmarker for Windows with an ultra-responsive **2026 Material 3 Dark** user interface. Built entirely in modern C++20 with raw Win32 Direct I/O.

---

## Key Features

- **Direct I/O Hardware Throughput**: Utilizes Win32 `FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH` with sector-aligned buffers (`VirtualAlloc`) to completely bypass the Windows OS file system cache. Measures physical storage speeds accurately.
- **Universal Windows Filesystem Support**: Automatically discovers and benchmarks drives formatted with any Windows-supported file system:
  - **NTFS**
  - **FAT32**
  - **exFAT**
  - **ReFS**
- **Dynamic Live Visualization (60 FPS)**:
  - Real-time speedometer gauge with adaptive max scaling.
  - Anti-aliased canvas wave chart plotting instantaneous and smoothed average speeds simultaneously.
  - Live IOPS (I/O Operations Per Second) and latency telemetry.
- **Drive & Test Selector**:
  - Full volume discovery (`GetLogicalDriveStringsW`, `GetVolumeInformationW`, `GetDiskFreeSpaceExW`).
  - Detailed capacity bar and sector size detection.
  - Configurable routines: **Sequential Write**, **Sequential Read**, **Random 4K (IOPS)**, or **Full Suite**.
  - Configurable file sizes (128 MB to 2 GB).
- **Standalone Portable Architecture**:
  - Zero external runtime DLLs required.
  - All web assets (HTML/CSS/JS) are embedded directly into the compiled executable.
  - Automatically launches as a borderless, focused native-like desktop application via browser App Mode.
  - Optional CLI mode for automated scripts (`--cli`).
- **Safe Execution & Cleanup**:
  - Non-destructive test files (`.rwbench_temp_benchmark.dat`) are automatically created, benchmarked, flushed, closed, and deleted immediately upon test completion or cancellation.

---

## User Interface Preview

The application incorporates cutting-edge **2026 Material 3 Dark** design principles:
- Deep obsidian surface container palette (`#090C10`, `#101520`, `#161C2B`).
- Vibrant neon accents: Electric Cyan (`#00E5FF`) for read throughput, Neon Emerald (`#00E676`) for write throughput, and Accent Violet (`#B388FF`).
- Glassmorphic panels with subtle micro-borders and responsive layout.
- Export results directly to structured JSON reports.

---

## Architecture & Technology Stack

| Component | Implementation |
|---|---|
| **Core Engine** | Modern C++20 with Win32 Storage API |
| **I/O Mechanism** | `CreateFileW` with `FILE_FLAG_NO_BUFFERING` |
| **Timing & Clock** | High-precision `QueryPerformanceCounter` (QPC) |
| **Embedded Server** | Pure Win32 Winsock (`ws2_32`) HTTP & REST daemon |
| **Frontend UI** | HTML5, CSS3, Vanilla JS (Material 3 Dark Design Tokens) |
| **Asset Storage** | C++ Raw String Literals (`src/embedded_assets.hpp`) |
| **Build & CI** | GitHub Actions (`windows-latest`, MSVC / MinGW) |

---

## Building from Source

All builds are automated through GitHub Actions CI/CD to prevent consuming local system resources.

### Automated CI/CD
Pushing a Git tag (e.g. `v1.0.0`) triggers the automated GitHub Actions pipeline which compiles the release binary and publishes it under GitHub Releases.

### Manual CMake Build (Windows)
```powershell
cmake -B build -DCMAKE_BUILD_TYPE=Release -A x64
cmake --build build --config Release
```
The resulting standalone executable will be located in `build/Release/rw-benchmark.exe`.

---

## Command Line Usage

To inspect detected volumes and test compatibility headlessly:
```powershell
rw-benchmark.exe --cli
```

---

## License

Copyright (c) 2026 milkycloud-dev. All rights reserved.  
Distributed under the **MilkyCloud Proprietary License**. See [LICENSE](LICENSE) for full legal terms.
