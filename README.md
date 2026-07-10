# CVFS Interface — Custom Virtual File System Explorer

**CVFS (Customized Virtual File System)** is an educational Linux-inspired filesystem implemented in C/C++ with a Qt-based graphical interface. It simulates the core internals of a real operating system filesystem — Super Block, Inode Table, File Descriptor Table, bitmaps, block allocation, and directory management — all visualized in real time.

> Built as a portfolio piece demonstrating systems programming, OS concepts, GUI design, and educational visualization.

---

## Quick Start — Download & Install

### Windows (One-Click Installer)

1. Go to the **Actions** tab of this repository
2. Click the latest successful workflow run (green checkmark)
3. Scroll down to **Artifacts**
4. Click **CVFS_Setup-Windows** to download the installer ZIP
5. Extract the ZIP and run `CVFS_Setup.exe`
6. Launch "CVFS File System Explorer" from the Start Menu

### Linux / macOS / Build from Source

See [Build Instructions](#build-instructions) below.

---

## Features

### File System Operations
- **Create, Read, Write, Delete** files with permission-based access (READ/WRITE/RW)
- **Open/Close** file descriptors with UFDT tracking
- **Seek** within files (START/CURRENT/END)
- **Directory management** — create, remove, list, navigate, path resolution
- **Truncate** files and query metadata (`stat`, `fstat`)
- **Persistent storage** — ext2-inspired virtual disk image (`cvfs.img`) with sync-on-write

### Real-Time Visualization
- **Live** inode bitmap grid (50 cells) — see allocations/deallocations as they happen
- **Live** block bitmap grid (502 cells) — watch data block allocation
- **Inode table viewer** with metadata columns (name, size, type, permission, link count)
- **Super Block panel** showing free inode/block counts
- **Step-by-step animation** — every file operation is broken into discrete steps played at configurable speed (0.1x – 4x)
- **Operation log** — textual step descriptions for learning

### System Information Panel
- Real-time panel showing super block stats, bitmap utilization percentages
- File property dialogs (right-click -> Properties)
- Directory tree with `QTreeView` multi-column layout

---

## Technology Stack

| Technology | Purpose | Version |
|------------|---------|---------|
| **C (C11/GNU C)** | Core file system engine (CVFS.cpp) | C11 |
| **C++ (C++17)** | Qt GUI application | C++17 |
| **Qt 6** | Cross-platform GUI framework | 6.6 |
| **CMake** | Build system generator | 3.16+ |
| **Inno Setup** | Windows installer creation | 6.x |
| **Git** | Version control | - |
| **GitHub** | Code hosting & collaboration | - |
| **GitHub Actions** | CI/CD (auto build & test) | - |
| **windeployqt** | Qt DLL deployment tool | (part of Qt) |
| **Emscripten** | WebAssembly compiler (optional) | 3.x |

---

## Architecture Overview

CVFS models the internal architecture of a Unix-like filesystem with three main layers:

**Layer 1 — Super Block**
The super block is the filesystem header. It stores the total number of inodes (50), the count of remaining free inodes, total data blocks (502), and free data blocks. It acts as the master metadata hub for the entire virtual disk.

**Layer 2 — Inode Management (DILB)**
The Disk Inode List Block (DILB) is a linked list of all inodes in memory. Each inode holds a file's name, size, type (file/directory), permission flags (READ/WRITE/RW), a pointer to its data buffer, link count (number of directory entries pointing to it), and reference count (number of open file descriptors).

**Layer 3 — File Descriptor System (UFDT + File Table)**
The User File Descriptor Table (UFDT) is an array of 50 entries. Each entry either points to a File Table structure or is NULL (free slot). The File Table stores per-open-instance metadata: read offset, write offset, access mode, and a back-pointer to the inode. This three-tier mapping (fd -> file table -> inode) mirrors how real operating systems manage open files.

| Component | Role |
|-----------|------|
| **Super Block** | Tracks total inodes (50) and free inode count |
| **DILB** (Disk Inode List Block) | Linked list of all inodes — each holds filename, size, type, permissions, data buffer pointer |
| **UFDT** (User File Descriptor Table) | Array of 50 entries, each pointing to a File Table entry (or NULL if free) |
| **File Table** | Per-open-instance metadata: read/write offsets, mode, back-pointer to inode |
| **Inode Bitmap** | Bit array tracking which inodes are allocated (1 = free, 0 = in use) |
| **Block Bitmap** | Bit array tracking which data blocks are allocated |
| **Inode Table (on disk)** | 56 on-disk inode slots (128 bytes each) with direct block pointers |

### Persistent Disk Layout

```
Block 0       Block 1       Block 2       Blocks 3-9        Blocks 10-511
+--------+   +--------+   +--------+   +------------+    +--------------+
| SUPER  |   | BLOCK  |   | INODE  |   |  INODE     |    |   DATA       |
| BLOCK  |   | BITMAP |   | BITMAP |   |  TABLE     |    |   BLOCKS     |
|        |   |        |   |        |   |  (56x128B) |    |   (502x1K)   |
+--------+   +--------+   +--------+   +------------+    +--------------+

Total: 512 blocks x 1024 bytes = 512 KB virtual disk
```

---

## Project Structure

| File/Directory | Purpose |
|----------------|---------|
| `CVFS.h` | Main header — all structures (SuperBlock, DiskInode, DirEntry, FileTable) and function prototypes |
| `CVFS.cpp` | Core implementation (~2500 lines) — all file system operations |
| `FileInfo.h/.cpp` | Helper classes for file metadata |
| `cvfs_cli.cpp` | Terminal-based shell (commands: ls, cd, mkdir, rm, cat, etc.) |
| `cvfs_original.cpp` | Original standalone CLI (pre-Qt) |
| `main.cpp` | Qt application entry point |
| `mainwindow.h/.cpp` | Main window with menus, toolbar, status bar |
| `filemanagerpanel.h/.cpp` | File explorer panel with tree view, table view, context menus |
| `filepropertydialog.h/.cpp` | File properties dialog |
| `newfiledialog.h/.cpp` | New file creation dialog |
| `cvfs_wrapper.h/.cpp` | C-to-C++ wrapper with step animation system |
| `cvfsmodel.h/.cpp` | Qt Model/View tree model for the file system |
| `helpdialog.h/.cpp` | Help/about dialog |
| `CMakeLists.txt` | Master build file |
| `.github/workflows/build.yml` | GitHub Actions CI/CD workflow |
| `cmake/toolchain_wasm.cmake` | Emscripten WebAssembly toolchain |
| `installer/CVFS_Setup.iss` | Inno Setup installer script |
| `scripts/package_windows.bat` | Windows packaging automation script |
| `README.md` | This file |
| `LICENSE` | MIT open-source license |
| `.gitignore` | Git ignore rules |

---

## Build Instructions

### Prerequisites

| Dependency | Minimum Version | Notes |
|------------|----------------|-------|
| CMake | 3.10+ | Build system |
| C++ Compiler | C++17 | GCC 8+, MSVC 2019+, Clang 10+ |
| Qt | 5.15 or 6.x | Widgets module (GUI only) |
| MinGW (Windows) | 8.0+ | Or MSVC toolchain |

### Quick Build (All Platforms)

```bash
# 1. Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 2. Build all targets
cmake --build build --config Release --parallel

# 3. Run the GUI
./build/cvfs_gui          # Linux/macOS
build/cvfs_gui.exe        # Windows

# OR run the CLI
./build/cvfs_cli          # Linux/macOS
build/cvfs_cli.exe        # Windows
```

### Build CLI Only (No Qt Required)

If Qt is not installed, CMake will automatically skip the GUI. You can also explicitly disable it:

```bash
cmake -B build -DCVFS_BUILD_GUI=OFF
cmake --build build
```

### Build GUI Only

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64
cmake --build build --target cvfs_gui
```

On Windows with MSVC:
```powershell
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --target cvfs_gui
```

### Build Outputs

After building, you get three executables:
- `cvfs_gui` — Qt GUI application (requires Qt)
- `cvfs_cli` — CLI version with persistent storage
- `cvfs_original` — Original standalone CLI (no persistence)

---

## Usage Guide

### GUI Mode (`cvfs_gui`)

The main window is split into two panels:
- **Left (File Manager):** Directory tree, file list with columns (Name, Size, Type, Permission, Inode), toolbar with navigation and file actions
- **Right (Visualization):** Super Block panel, Inode Bitmap grid, Block Bitmap, Inode Table, Operation Log

**Operations:**
- **New File:** Right-click -> New File, or toolbar `+File` button
- **New Directory:** Right-click -> New Folder, or toolbar `+Folder` button
- **Delete:** Right-click -> Delete (or select + toolbar Delete)
- **Rename:** Right-click -> Rename (or inline edit with F2)
- **Open/View:** Double-click a file to open in the content viewer
- **Properties:** Right-click -> Properties to see full inode metadata
- **Visualization Speed:** Adjust playback speed (0.1x to 4x) for step animation

### CLI Mode (`cvfs_cli`)

```
CVFS Interactive Shell
Commands:
  create <name> <perm>     Create file (perm: 1=READ, 2=WRITE, 3=RW)
  open <name> <mode>       Open file
  write <fd> <data>        Write data to file
  read <fd> <size>         Read from file
  close <name|fd>          Close file
  rm <name>                Delete file
  ls                       List files
  stat <name>              Show file metadata
  fstat <fd>               Show file metadata by fd
  truncate <name>          Truncate file
  seek <fd> <pos> <from>   Seek (from: 0=START, 1=CURRENT, 2=END)
  mkdir <name>             Create directory
  rmdir <name>             Remove directory
  cd <path>                Change directory
  show                     Show inode/UFDT state
  help                     Display help
  exit                     Exit
```

---

## CI/CD — GitHub Actions

Every push to the `main` branch automatically triggers a build pipeline defined in `.github/workflows/build.yml`.

### Pipeline Jobs

**Job 1: build-linux (Ubuntu)**
- Installs Qt 6.6
- Configures with CMake (Unix Makefiles)
- Builds all targets (CLI + GUI)
- Uploads Linux executables as artifacts

**Job 2: build-windows (Windows Server)**
- Installs Qt 6.6 for MSVC 2019 64-bit
- Builds all targets in Release mode
- Runs `windeployqt` to collect Qt DLLs
- Builds the Inno Setup installer (`CVFS_Setup.exe`)
- Uploads the installer as `CVFS_Setup-Windows` artifact

### Download the Installer

1. Go to the **Actions** tab
2. Click the latest successful run (green checkmark)
3. Scroll to **Artifacts** section
4. Click **CVFS_Setup-Windows** to download
5. Extract the ZIP, run `CVFS_Setup.exe`

---

## Windows Installer (Inno Setup)

The installer is built using **Inno Setup**, a free tool for creating professional Windows installers.

### What the Installer Includes
- `cvfs_gui.exe` — Main application
- All required Qt DLLs (collected by windeployqt)
- Platform plugins, styles, icon engines, image formats
- Start Menu shortcuts (GUI, CLI, Uninstall)
- Optional Desktop shortcut
- File association for `.cvfs` extension
- Clean uninstall

### Build the Installer Locally

```bash
# Install Inno Setup from https://jrsoftware.org/isdl.php
# Then run the packaging script:
scripts\package_windows.bat
```

Or manually:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
windeployqt build\Release\cvfs_gui.exe --release
ISCC installer\CVFS_Setup.iss
```

---

## Bug Fix History

### Bug 1: Context Menu Not Working (Right-Click)
**Symptom:** Right-clicking a file and selecting Open/Delete/Rename did nothing.
**Root cause:** `onCustomContextMenu()` never called `setCurrentIndex()`, so action handlers used stale `currentIndex()`.
**Fix:** Added `m_treeView->setCurrentIndex(index)` before showing the context menu.
**File:** `gui/filemanagerpanel.cpp`

### Bug 2: Double-Click Not Opening Files
**Symptom:** Double-clicking a file showed "unable to open file" error.
**Root cause:** `onItemDoubleClicked` correctly identified the inode but then called `onOpen()` which re-fetched from `currentIndex()` instead of the clicked index.
**Fix:** Call `m_model->openFile(index)` directly instead of `onOpen()`.
**File:** `gui/filemanagerpanel.cpp`

### Bug 3: Step Messages Showed Raw %d / %s
**Symptom:** Animation messages displayed literal "%d" and "%s" instead of actual values.
**Root cause:** `fireStep()` was called with printf-style format strings but no arguments were substituted.
**Fix:** Added variadic `fireStepF()` helper (using `<cstdarg>` + `vsnprintf`) and rewrote all 45+ fireStep calls with real arguments.
**File:** `cvfs_wrapper.cpp`

### Bug 4: Data Blocks Not Freed on Delete
**Symptom:** Deleting files did not free their data blocks, causing disk to fill up over time.
**Root cause:** `rm_File()` freed the inode but never called `free_data_block()` for the file's data blocks.
**Fix:** Read the old DiskInode from disk and call `free_data_block()` on each `i_block[]` entry before clearing the inode.
**File:** `CVFS.cpp`

### Bug 5: Wrong Block Numbers in Step Animation
**Symptom:** Delete/truncate animation highlighted blocks 0,1,2,3 instead of the actual file blocks.
**Root cause:** `W_rm_File()` and `W_truncate_File()` used loop counter `i` instead of actual DiskInode block numbers.
**Fix:** Read DiskInode before the loop and pass real block numbers to `fireStep()`.
**File:** `cvfs_wrapper.cpp`

---

## Data Structures Reference

### Super Block (in-memory)
```c
typedef struct superblock {
    int TotalInodes;   // Total inodes (50)
    int FreeInode;     // Currently free inodes
} SUPERBLOCK;
```

### Inode (in-memory)
```c
typedef struct inode {
    char FileName[50];     // File name
    int InodeNumber;       // Unique inode number (1-50)
    int FileSize;          // Maximum size (2048)
    int FileActualSize;    // Actual data size
    int FileType;          // 0=free, 1=regular, 2=directory
    char *Buffer;          // Heap-allocated data
    int LinkCount;         // Directory entry count
    int ReferenceCount;    // Open FD count
    int permission;        // 1=READ, 2=WRITE, 3=RW
    struct inode *next;    // Linked list pointer
} INODE;
```

### File Table & UFDT
```c
typedef struct filetable {
    int readoffset;
    int writeoffset;
    int count;
    int mode;           // 1=READ, 2=WRITE, 3=RW
    PINODE ptrinode;    // Pointer to inode
} FILETABLE;

typedef struct ufdt {
    PFILETABLE ptrfiletable;  // NULL = free entry
} UFDT;  // UFDTArr[50]
```

### On-Disk Structures
```c
struct DiskSuperBlock {
    int s_inodes_count;       // Total inodes (50)
    int s_free_inodes_count;  // Free inodes
    int s_blocks_count;       // Data blocks (502)
    int s_free_blocks_count;  // Free data blocks
    int s_first_data_block;   // Block 10
    int s_block_size;         // 1024
    int s_magic;              // 0xEF53
    char reserved[996];
};

struct DiskInode {
    char i_filename[48];
    int  i_mode;              // Type + permissions
    int  i_size;              // Actual file size
    int  i_links_count;
    int  i_blocks_count;
    int  i_block[4];          // Direct block pointers
    char i_reserved[48];
};

struct DirEntry {
    int  d_inode;
    char d_name[59];
    char d_type;              // 1=file, 2=directory
};
```

---

## Educational Value

CVFS is designed to make abstract OS filesystem concepts tangible:

- **See** inode allocation/deallocation in real time on a bitmap grid
- **Watch** data blocks get reserved and freed as files are created/deleted
- **Trace** the UFDT life cycle — how file descriptors map to file tables to inodes
- **Understand** the Super Block's role as the filesystem metadata hub
- **Follow** step-by-step animations that decompose operations like "Create File" into 7 discrete internal steps
- **Experiment** with permissions, linking, and directory structures in a sandboxed environment

---

## FAQ

**Q: The GUI says "unable to open file" when I double-click or right-click.**
A: This was a known bug that has been fixed. Pull the latest code or download the latest installer.

**Q: GitHub Actions keeps failing.**
A: Check the Actions tab for the error message. Common causes: (a) Qt installation issues — try re-running; (b) Inno Setup script errors; (c) Check Settings -> Actions -> General -> "Allow all actions" is enabled.

**Q: How do I create a fresh virtual disk?**
A: Delete the `cvfs.img` file in the same folder as the executable. It will be recreated automatically.

**Q: Can I change the disk size?**
A: Yes — modify `MAX_BLOCKS`, `BLOCK_SIZE`, and `DATA_START_BLOCK` in `CVFS.h`. Note that existing `cvfs.img` files will become incompatible.

**Q: What is the maximum file size?**
A: Each file can use up to 4 data blocks (DiskInode has 4 `i_block` entries). With 1 KB blocks, maximum is 4 KB.

**Q: Why doesn't the GUI build?**
A: Qt 6 must be installed and found by CMake. Run `cmake -B build` and check if it says "Qt6 found: YES". If not, install Qt or set `CMAKE_PREFIX_PATH`.

---

## License

Distributed under the MIT License. See `LICENSE` for more information.
