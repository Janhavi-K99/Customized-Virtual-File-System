# CVFS Interface — Custom Virtual File System Explorer

**CVFS (Customized Virtual File System)** is an educational Linux-inspired filesystem implemented in C++ with a Qt-based graphical interface. It simulates the core internals of a real operating system filesystem — Super Block, Inode Table, File Descriptor Table, bitmaps, block allocation, and directory management — all visualized in real time.

> Built as a portfolio piece demonstrating systems programming, OS concepts, GUI design, and educational visualization.

<!--
TODO: Add screenshot here
![CVFS Interface Screenshot](screenshots/cvfs_main.png)
-->

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
- **Step-by-step animation** — every file operation is broken into discrete steps played at configurable speed (0.1× – 4×)
- **Operation log** — textual step descriptions for learning

### System Information Panel
- Real-time panel showing super block stats, bitmap utilization percentages
- File property dialogs (right-click → Properties)
- Directory tree with `QTreeView` multi-column layout

---

## Architecture Overview

CVFS models the internal architecture of a Unix-like filesystem:

```
┌──────────────────────────────────────────────────────────────┐
│                        SUPER BLOCK                            │
│               TotalInodes: 50  |  FreeInodes: N               │
└──────────────────────────────────────────────────────────────┘
         │                                    │
         ▼                                    ▼
┌──────────────────────┐    ┌──────────────────────────────┐
│  DILB (Inode List)   │    │  UFDT (File Descriptor Table) │
│  ┌──────┐ ┌──────┐   │    │  [0] ──→ FILE TABLE          │
│  │INODE1│→│INODE2│→...│    │  [1] ──→ FILE TABLE          │
│  └──────┘ └──────┘   │    │  ...                          │
│  FileName, Size,      │    │  [49]                         │
│  FileType, Buffer,    │    └──────────────────────────────┘
│  Link/Reference Count │                  │
└──────────────────────┘                  │ ptrinode
         │                                │
         └────────────────────────────────┘
                    │
                    ▼
         ┌──────────────────┐
         │    FILE TABLE     │
         │  readoffset       │
         │  writeoffset      │
         │  mode (R/W/RW)    │
         │  ptrinode ────────┤
         └──────────────────┘
```

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
Block 0       Block 1       Block 2       Blocks 3–9        Blocks 10–511
┌────────┐   ┌────────┐   ┌────────┐   ┌────────────┐    ┌──────────────┐
│ SUPER  │   │ BLOCK  │   │ INODE  │   │  INODE     │    │   DATA       │
│ BLOCK  │   │ BITMAP │   │ BITMAP │   │  TABLE     │    │   BLOCKS     │
│        │   │        │   │        │   │  (56×128B) │    │   (502×1K)   │
└────────┘   └────────┘   └────────┘   └────────────┘    └──────────────┘

Total: 512 blocks × 1024 bytes = 512 KB virtual disk
```

---

## Build Instructions

### Prerequisites

| Dependency | Minimum Version | Notes |
|------------|----------------|-------|
| CMake | 3.10+ | Build system |
| C++ Compiler | C++17 | GCC 8+, MSVC 2019+, Clang 10+ |
| Qt | 5.15 or 6.x | Widgets module (GUI only) |
| MinGW (Windows) | 8.0+ | Or MSVC toolchain |

### Build CLI (no Qt required)

```bash
# Configure
cmake -B build

# Build CLI executable
cmake --build build --target cvfs_cli

# Run
./build/cvfs_cli
```

### Build GUI (requires Qt)

```bash
# Configure with Qt path (if not in PATH)
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64

# Build GUI
cmake --build build --target cvfs_gui

# Run
./build/cvfs_gui
```

On Windows with MSVC:

```powershell
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --target cvfs_gui
```

### Build All Targets

```bash
cmake --build build
```

This produces three executables:
- `cvfs_cli` — CLI version with persistent storage
- `cvfs_original` — Original standalone CLI (no persistence)
- `cvfs_gui` — Qt GUI application

---

## Usage Guide

### GUI Mode (`cvfs_gui`)

The main window is split into two panels:
- **Left (File Manager):** Directory tree, file list with columns (Name, Size, Type, Permission, Inode), toolbar with navigation and file actions
- **Right (Visualization):** Super Block panel, Inode Bitmap grid, Block Bitmap, Inode Table, Operation Log

**Operations:**
- **New File:** Right-click → New File, or toolbar `+File` button
- **New Directory:** Right-click → New Folder, or toolbar `+Folder` button
- **Delete:** Right-click → Delete (or select + toolbar Delete)
- **Rename:** Right-click → Rename (or inline edit with F2)
- **Open/View:** Double-click a file to open in the content viewer
- **Properties:** Right-click → Properties to see full inode metadata
- **Visualization Speed:** Adjust playback speed (0.1× to 4×) for step animation

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
    int InodeNumber;       // Unique inode number (1–50)
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

The BLUEPRINT.md document provides a complete architectural deep-dive spanning:
- In-memory vs. on-disk structure mapping
- Operation flowcharts for every system call
- Full disk layout with ext2-inspired addressing
- Complete operation walkthroughs with end-to-end tracing
- Qt Model/View bridge pattern documentation
- Visualization engine architecture with timing and state management

---

## License

Distributed under the MIT License. See `LICENSE` for more information.
