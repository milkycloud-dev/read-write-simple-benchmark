# UI and Visual Verification Rules

## 1. Typography and DPI Independence
- Always use `Gdiplus::UnitPixel` (never `Gdiplus::UnitPoint`) for Win32 and GDI+ desktop user interface text and layout dimensions.
- Calling `SetProcessDPIAware()` is mandatory at application startup to ensure 1:1 pixel coordinate parity across 100%, 125%, 150%, and 200% Windows display scaling without OS font ballooning or letter collisions.
- Use `StringFormat::GenericTypographic()` when measuring text bounding boxes via `MeasureString` to eliminate artificial 1/6 em character padding.

## 2. Headless Offscreen Screenshot Interface
- Every native desktop graphical application must support a headless `--screenshot <path>` (or `-s <path>`) command-line flag.
- The screenshot mode must render directly to an in-memory `Gdiplus::Bitmap` buffer (`PixelFormat32bppARGB`) via `Graphics` and save via PNG encoder without requiring an interactive desktop session or `CreateWindowEx` window display.
- Crucial lifecycle constraint: Always scope the `Gdiplus::Graphics` object inside a block so its destructor executes before calling `Bitmap::Save()`. GDI+ holds an active surface lock while `Graphics` is alive, which causes `Bitmap::Save()` to fail with `ObjectBusy`.

## 3. CI/CD Pipeline Verification
- In CI workflows (e.g. GitHub Actions Windows runners), run the executable with `--screenshot preview.png`.
- In PowerShell, run GUI subsystem applications (`/SUBSYSTEM:WINDOWS`) using `Start-Process -FilePath $exe -ArgumentList @("--screenshot", "`"$fullPath`"") -Wait -PassThru -NoNewWindow` so the shell synchronously waits for execution and captures the true exit code.
- Always publish the generated `preview.png` as a CI build/release artifact.

## 4. Autonomous Agent Visual Inspection
- Never pass an untested UI layout to the user.
- Before reporting completion on UI tasks, download the compiled binary, run `--screenshot`, and inspect the resulting PNG image autonomously via `view_file`.
- Personally confirm that zero text collisions, overlapping glyphs, baseline jumps, or dead voids exist in the rendered pixels before delivering to the user.
