# CVFS — Customized Virtual File System

CVFS is an educational Linux-inspired virtual file system built in C with a Qt-based GUI. It simulates how a real operating system manages files — inodes, super blocks, bitmaps, file descriptors, directories, and block allocation — all visualized in real time on a virtual 512 KB disk.

---

## Tech Stack

| Technology | Purpose |
|------------|---------|
| **C (C11)** | Core file system engine |
| **C++ (C++17)** | GUI application |
| **Qt 6** | Cross-platform GUI framework |
| **CMake** | Build system |
| **Inno Setup** | Windows installer |
| **GitHub Actions** | Auto build & package |

---

## How to Download

### Windows (One-Click Installer)

1. Go to the **Actions** tab of this repository
2. Click the latest successful run (green checkmark)
3. Scroll down to **Artifacts**
4. Click **CVFS_Setup-Windows** to download the ZIP
5. Extract and run `CVFS_Setup.exe`

### Build from Source (Linux / macOS / Windows)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
./build/cvfs_gui        # or build/cvfs_gui.exe on Windows
```

---

## How to Use

### GUI Mode

The window has two sides:
- **Left panel** — file tree and file list. Right-click for New File, New Folder, Delete, Rename, Properties. Double-click to open a file.
- **Right panel** — live visualizations: inode bitmap (50 cells), block bitmap (502 cells), super block stats, inode table, and step-by-step operation log.

Use the speed slider to control animation speed (0.1x to 4x).

### CLI Mode

Run `cvfs_cli` for a terminal shell with commands:

| Command | What it does |
|---------|-------------|
| `create <name> <perm>` | Create a file (perm: 1=READ, 2=WRITE, 3=RW) |
| `open <name> <mode>` | Open a file |
| `write <fd> <data>` | Write data |
| `read <fd> <size>` | Read from file |
| `close <name>` | Close a file |
| `rm <name>` | Delete a file |
| `ls` | List files |
| `stat <name>` | Show file info |
| `truncate <name>` | Empty a file |
| `mkdir <name>` / `rmdir <name>` | Create/remove directory |
| `cd <path>` | Change directory |
| `help` | Show all commands |
| `exit` | Quit |

---

## How to Understand It

The file system has three layers:

**1. Super Block** — The disk header. Stores total inodes (50), free inodes, data blocks (502), and free blocks.

**2. Inodes (DILB)** — Each file/directory has an inode with its name, size, type, permissions, and pointers to its data blocks. Inodes are tracked in a linked list in memory and saved to disk.

**3. File Descriptors (UFDT + File Table)** — When you open a file, the UFDT allocates an entry that points to a File Table structure, which in turn points to the inode. This three-level mapping (fd → file table → inode) is how real OSes manage open files.

**Disk Layout:**
- Block 0: Super Block
- Block 1: Block Bitmap
- Block 2: Inode Bitmap
- Blocks 3–9: Inode Table (56 inodes x 128 bytes)
- Blocks 10–511: Data Blocks (502 blocks x 1 KB)

Total: 512 blocks x 1024 bytes = 512 KB virtual disk stored in `cvfs.img`.

The right panel in the GUI shows all of this live — create a file and watch the inode bitmap, block bitmap, and super block update in real time with step-by-step animation.

---

## Author

**Janhavi Kolamkar**
