# Animated Wallpapers Reimagined

An ultra-lean, native Windows wallpaper engine built in pure C that hooks directly into the desktop environment (`WorkerW`), utilizing hardware-accelerated Media Foundation pipelines for 0% idle CPU usage and flat memory footprints.

---

## Prerequisites & Setup

Before building, ensure you have your chosen toolchain correctly mapped to your environment variables:

* **For GCC:** Open your **MSYS2** terminal and switch to the **UCRT64** environment.
* **For Clang/MSVC:** Open the native x64 or x86 **Developer Command Prompt for VS**.

> **Important Deployment Step:** Place your target video file directly inside the executable directory and name it exactly **`video.mp4`** before launching the compiled application.

---

## Project Layout

Ensure your source files are organized inside your project directory as follows:

```text
AnimatedWallpapers2.0/
├── config.h
├── resource.rc
└── main.c
```

---

## Compilation Workflows

### Method 1: Building with GCC (MinGW-w64 via MSYS2 UCRT64)
GCC compiles the Windows resource script into a standard object file (`resource.o`) before performing final linking. The `-s` flag is explicitly applied to strip symbol tables and keep the executable footprint down to a minimum.

Run the following commands sequentially in your terminal:

```bash
# 1. Compile the metadata resource sheet into a GNU binary object
windres resource.rc -o resource.o

# 2. Compile the core application code and link system dependencies
gcc main.c resource.o -O2 -s -mwindows -luser32 -lole32 -loleaut32 -luuid -lmfplat -lmfuuid -o "Animated Wallpapers Reimagined.exe"
```

### Method 2: Building with Clang + MSVC Toolchain (`clang-cl`)
When utilizing the MSVC ecosystem, Clang invokes the Microsoft resource compiler (`rc.exe`) to pack binary configuration blocks into a `.res` file, optimizing for space with `/O1`.

Run the following commands sequentially in your Developer Command Prompt:

```cmd
:: 1. Compile the windows resource script into a binary resource table
rc resource.rc

:: 2. Link compilation parameters seamlessly via the Clang compiler front-end
clang-cl main.c resource.res /O1 /MD /link /SUBSYSTEM:WINDOWS user32.lib ole32.lib oleaut32.lib uuid.lib mfplat.lib mfuuid.lib /OUT:"Animated Wallpapers Reimagined.exe"
```

---

## Verification & Control

1. **Verify Branding:** Once compiled, right-click the generated `.exe`, open **Properties**, and look at the **Details** tab. All fields—including Company, Creator, Product Name, and Copyright—will be cleanly populated with **OXOP** metadata.
2. **Execution:** Double-click the file. The window will securely parent itself underneath your desktop icons without throwing background processing lags or memory accumulation leaks.
3. **Graceful Exit:** To close the application and return instantly to your standard static Windows background without ghost frames, locate `Animated Wallpapers Reimagined.exe` inside Task Manager and select **End Task**.
