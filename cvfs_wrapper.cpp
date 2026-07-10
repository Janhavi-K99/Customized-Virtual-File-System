#include "cvfs_wrapper.h"
#include "persistence.h"
#include "directory.h"
#include <cstdarg>
#include <cstdio>

PreOpHook g_preOpHook = NULL;
PostOpHook g_postOpHook = NULL;
StepHook g_stepHook = NULL;

static void fireStep(const char *comp, const char *action, int tgt, int extra,
                     const char *op, const char *file, int pct, const char *expl)
{
    if (g_stepHook) g_stepHook(comp, action, tgt, extra, op, file, pct, expl);
}

static void fireStepF(const char *comp, const char *action, int tgt, int extra,
                      const char *op, const char *file, int pct, const char *fmt, ...)
{
    if (!g_stepHook) return;
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    g_stepHook(comp, action, tgt, extra, op, file, pct, buf);
}

void CVFS_Init()
{
    InitialiseSuperBlock();
    CreateDILB();
    mount_virtual_disk(CVFS_DISK_PATH);
}

void CVFS_Shutdown()
{
    unmount_virtual_disk();
}

int W_CreateFile(char *name, int permission)
{
    if (g_preOpHook) g_preOpHook("create", name, permission, 0);

    fireStep("SuperBlock", "check", -1, -1, "create", name, 5,
        "Checking Super Block for available inode slot...");
    fireStep("InodeBitmap", "scan", -1, -1, "create", name, 15,
        "Scanning Inode Bitmap for a free inode (a bit set to 0)...");

    int result = CreateFile(name, permission);

    if (result >= 0) {
        int inodeNum = UFDTArr[result].ptrfiletable->ptrinode->InodeNumber;

        fireStepF("InodeBitmap", "allocate", inodeNum - 1, -1, "create", name, 30,
            "Allocating inode #%d in the Inode Bitmap (marking bit as 0)...", inodeNum);
        fireStepF("InodeTable", "update", inodeNum - 1, -1, "create", name, 45,
            "Writing file metadata into inode #%d: name, type=REGULAR, permission, size=0...", inodeNum);

        dir_add_entry(dir_get_cwd_inode(), inodeNum, name, REGULAR);

        fireStepF("Directory", "add", inodeNum - 1, -1, "create", name, 60,
            "Adding directory entry: filename '%s' maps to inode #%d in current directory...", name, inodeNum);
        fireStep("SuperBlock", "update", -1, -1, "create", name, 75,
            "Decrementing free inode count in Super Block...");

        sync_inode(inodeNum);
        sync_inode(dir_get_cwd_inode());
        sync_superblock();

        fireStep("DiskSync", "sync", inodeNum - 1, -1, "create", name, 90,
            "Synchronizing inode, directory, and Super Block to virtual disk...");
    } else {
        fireStep("", "error", -1, -1, "create", name, 0,
            "Create failed: no free inode or name conflict.");
    }

    fireStepF("", "complete", -1, -1, "create", name, 100,
        "File '%s' created and ready for read/write operations.", name);
    if (g_postOpHook) g_postOpHook("create", name, permission, 0, result);
    return result;
}

int W_rm_File(char *name)
{
    if (g_preOpHook) g_preOpHook("delete", name, 0, 0);

    fireStepF("Directory", "scan", -1, -1, "delete", name, 5,
        "Scanning directory entries to locate filename '%s'...", name);

    int fd = GetFDFromName(name);
    int inodeNum = -1;
    PINODE node = FindFile(name);
    if (node) inodeNum = node->InodeNumber;

    int blockNums[4];
    int nBlocks = 0;

    if (inodeNum > 0) {
        fireStepF("InodeTable", "read", inodeNum - 1, -1, "delete", name, 20,
            "Reading inode #%d: checking file size and link count...", inodeNum);

        DiskInode tmpInode;
        if (read_inode(inodeNum, &tmpInode) == 0) {
            nBlocks = tmpInode.i_blocks_count;
            if (nBlocks > 4) nBlocks = 4;
            for (int i = 0; i < nBlocks; i++)
                blockNums[i] = tmpInode.i_block[i];
        }

        for (int i = 0; i < nBlocks; i++) {
            int blk = blockNums[i];
            int bmpIdx = (blk >= DATA_START_BLOCK) ? blk - DATA_START_BLOCK : i;
            fireStepF("BlockBitmap", "free", bmpIdx, -1, "delete", name, 30 + i * 10,
                "Freeing data block #%d in Block Bitmap \u2014 content no longer needed...", blk);
        }
    }

    int result = rm_File(name);
    if (result >= 0) {
        fireStepF("InodeBitmap", "free", inodeNum - 1, -1, "delete", name, 75,
            "Marking inode #%d as free in the Inode Bitmap...", inodeNum);
        fireStepF("Directory", "remove", -1, -1, "delete", name, 85,
            "Removing directory entry for '%s' from parent directory...", name);

        dir_remove_entry(dir_get_cwd_inode(), name);

        if (inodeNum > 0) sync_inode(inodeNum);
        sync_inode(dir_get_cwd_inode());
        sync_superblock();

        fireStep("DiskSync", "sync", -1, -1, "delete", name, 95,
            "Synchronizing freed inode, bitmap, and directory to virtual disk...");
    }

    fireStepF("", "complete", -1, -1, "delete", name, 100,
        "File '%s' deleted. Inode, blocks, and directory entry freed.", name);
    if (g_postOpHook) g_postOpHook("delete", name, 0, 0, result);
    return result;
}

int W_ReadFile(int fd, char *arr, int isize)
{
    if (g_preOpHook) g_preOpHook("read", NULL, fd, isize);
    if (fd >= 0 && fd < 50 && UFDTArr[fd].ptrfiletable) {
        int inodeNum = UFDTArr[fd].ptrfiletable->ptrinode->InodeNumber;
        const char *fname = UFDTArr[fd].ptrfiletable->ptrinode->FileName;
        int mode = UFDTArr[fd].ptrfiletable->mode;
        int roff = UFDTArr[fd].ptrfiletable->readoffset;
        int fsize = UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize;

        fireStepF("UFDT", "lookup", fd, inodeNum - 1, "read", fname, 20,
            "Looking up file descriptor #%d in UFDT \u2014 finding its FileTable entry...", fd);
        fireStepF("FileTable", "read", fd, inodeNum - 1, "read", fname, 40,
            "Reading FileTable entry: mode=%d, read offset=%d, inode=#%d...", mode, roff, inodeNum);
        fireStepF("InodeTable", "read", inodeNum - 1, fd, "read", fname, 60,
            "Reading inode #%d metadata: file size=%d bytes, checking read permissions...", inodeNum, fsize);
        fireStepF("DataBlocks", "read", inodeNum - 1, fd, "read", fname, 80,
            "Copying %d bytes from inode buffer into user-provided buffer...", isize);
    }
    int result = -1;
    if (fd >= 0 && fd < 50 && UFDTArr[fd].ptrfiletable)
        result = ReadFile(fd, arr, isize);
    fireStepF("", "complete", -1, -1, "read", "", 100, "Read complete: %d bytes transferred.", result > 0 ? result : 0);
    if (g_postOpHook) g_postOpHook("read", NULL, fd, isize, result);
    return result;
}

int W_WriteFile(int fd, char *arr, int isize)
{
    if (g_preOpHook) g_preOpHook("write", NULL, fd, isize);
    if (fd >= 0 && fd < 50 && UFDTArr[fd].ptrfiletable) {
        int inodeNum = UFDTArr[fd].ptrfiletable->ptrinode->InodeNumber;
        const char *fname = UFDTArr[fd].ptrfiletable->ptrinode->FileName;
        int mode = UFDTArr[fd].ptrfiletable->mode;
        int woff = UFDTArr[fd].ptrfiletable->writeoffset;
        int oldSize = UFDTArr[fd].ptrfiletable->ptrinode->FileActualSize;

        fireStepF("UFDT", "lookup", fd, inodeNum - 1, "write", fname, 10,
            "Looking up file descriptor #%d...", fd);
        fireStepF("FileTable", "write", fd, inodeNum - 1, "write", fname, 25,
            "Checking FileTable: mode=%d permits write, write offset=%d...", mode, woff);
        fireStepF("InodeTable", "read", inodeNum - 1, fd, "write", fname, 40,
            "Reading inode #%d: current size=%d, buffer ready for %d new bytes...", inodeNum, oldSize, isize);

        int newSize = oldSize + isize;
        int oldBlocks = (oldSize + 1023) / 1024;
        int newBlocks = (newSize + 1023) / 1024;

        for (int b = oldBlocks; b < newBlocks && b < 4; b++) {
            fireStepF("BlockBitmap", "allocate", b, -1, "write", fname, 50 + (b - oldBlocks) * 8,
                "Allocating new file-block #%d (next free data block) for the expanded file content...", b);
        }
    }

    int result = -1;
    if (fd >= 0 && fd < 50 && UFDTArr[fd].ptrfiletable)
        result = WriteFile(fd, arr, isize);
    if (result > 0 && fd >= 0 && fd < 50 && UFDTArr[fd].ptrfiletable) {
        int inodeNum = UFDTArr[fd].ptrfiletable->ptrinode->InodeNumber;
        const char *fname = UFDTArr[fd].ptrfiletable->ptrinode->FileName;

        fireStepF("DataBlocks", "write", inodeNum - 1, result, "write", fname, 75,
            "Writing %d bytes into data blocks via inode buffer...", result);
        fireStepF("InodeTable", "update", inodeNum - 1, result, "write", fname, 85,
            "Updating inode #%d: new file size=%d bytes...", inodeNum, result);
        fireStep("DiskSync", "sync", -1, -1, "write", fname, 95,
            "Synchronizing updated inode to virtual disk...");

        sync_inode(inodeNum);
    }
    fireStepF("", "complete", -1, -1, "write", "", 100, "Write complete: %d bytes written.", result > 0 ? result : 0);
    if (g_postOpHook) g_postOpHook("write", NULL, fd, isize, result);
    return result;
}

int W_OpenFile(char *name, int mode)
{
    if (g_preOpHook) g_preOpHook("open", name, mode, 0);

    fireStepF("Directory", "find", -1, -1, "open", name, 20,
        "Searching directory entries for filename '%s'...", name);

    int result = OpenFile(name, mode);
    if (result >= 0) {
        int inodeNum = UFDTArr[result].ptrfiletable->ptrinode->InodeNumber;
        fireStepF("InodeTable", "read", inodeNum - 1, mode, "open", name, 50,
            "Verifying inode #%d permissions (mode=%d) and file type...", inodeNum, mode);
        fireStepF("UFDT", "allocate", result, inodeNum - 1, "open", name, 75,
            "Allocating UFDT entry #%d \u2014 creating FileTable with mode=%d, offsets=0...", result, mode);
        fireStepF("FileTable", "init", result, mode, "open", name, 90,
            "FileTable initialized: readoffset=0, writeoffset=0, ptrinode=#%d.", inodeNum);
    }
    fireStepF("", "complete", -1, -1, "open", name, 100,
        "File '%s' opened. File descriptor=%d.", name, result);
    if (g_postOpHook) g_postOpHook("open", name, mode, 0, result);
    return result;
}

int W_CloseFileByName(char *name)
{
    if (g_preOpHook) g_preOpHook("close", name, 0, 0);

    fireStepF("UFDT", "find", -1, -1, "close", name, 25,
        "Finding file descriptor for '%s' in UFDT...", name);

    int fd = GetFDFromName(name);
    if (fd >= 0 && UFDTArr[fd].ptrfiletable) {
        int inodeNum = UFDTArr[fd].ptrfiletable->ptrinode->InodeNumber;
        fireStepF("FileTable", "reset", fd, inodeNum - 1, "close", name, 50,
            "Resetting FileTable offsets for fd=%d, decrementing inode reference count...", fd);
        fireStepF("InodeTable", "update", inodeNum - 1, fd, "close", name, 75,
            "Inode #%d reference count decreased. File table entry freed.", inodeNum);
        fireStepF("UFDT", "free", fd, inodeNum - 1, "close", name, 90,
            "Freeing UFDT entry #%d \u2014 slot returned to available pool.", fd);
    }
    int result = CloseFileByName(name);
    fireStepF("", "complete", -1, -1, "close", name, 100, "File '%s' closed.", name);
    if (g_postOpHook) g_postOpHook("close", name, 0, 0, result);
    return result;
}

void W_CloseAllFile()
{
    if (g_preOpHook) g_preOpHook("closeall", NULL, 0, 0);
    fireStep("UFDT", "scan", -1, -1, "closeall", "", 20,
        "Scanning all 50 UFDT entries for open files...");
    int count = 0;
    for (int i = 0; i < 50; i++)
        if (UFDTArr[i].ptrfiletable) count++;
    fireStepF("UFDT", "free", -1, count, "closeall", "", 80,
        "Closing %d open file descriptors \u2014 freeing all FileTable entries...", count);
    CloseAllFile();
    fireStep("", "complete", -1, -1, "closeall", "", 100, "All file descriptors closed.");
    if (g_postOpHook) g_postOpHook("closeall", NULL, 0, 0, 0);
}

int W_LseekFile(int fd, int size, int from)
{
    if (g_preOpHook) g_preOpHook("lseek", NULL, fd, size);
    const char *origin = (from == 0) ? "SET" : (from == 1) ? "CUR" : "END";
    fireStepF("FileTable", "seek", fd, from, "lseek", "", 40,
        "Adjusting file offset: fd=%d, delta=%d, origin=%s...", fd, size, origin);
    int result = LseekFile(fd, size, from);
    fireStep("", "complete", -1, -1, "lseek", "", 100, "Seek complete. New offset set.");
    if (g_postOpHook) g_postOpHook("lseek", NULL, fd, size, result);
    return result;
}

int W_truncate_File(char *name)
{
    if (g_preOpHook) g_preOpHook("truncate", name, 0, 0);

    fireStepF("Directory", "find", -1, -1, "truncate", name, 10,
        "Finding '%s' in directory entries...", name);

    PINODE node = FindFile(name);
    int blockNums[4];
    int nBlocks = 0;

    if (node) {
        int inodeNum = node->InodeNumber;
        fireStepF("InodeTable", "read", inodeNum - 1, node->FileActualSize, "truncate", name, 25,
            "Reading inode #%d: current size=%d bytes, preparing to clear...", inodeNum, node->FileActualSize);

        DiskInode tmpInode;
        if (read_inode(inodeNum, &tmpInode) == 0) {
            nBlocks = tmpInode.i_blocks_count;
            if (nBlocks > 4) nBlocks = 4;
            for (int i = 0; i < nBlocks; i++)
                blockNums[i] = tmpInode.i_block[i];
        }

        for (int i = 0; i < nBlocks; i++) {
            int blk = blockNums[i];
            int bmpIdx = (blk >= DATA_START_BLOCK) ? blk - DATA_START_BLOCK : i;
            fireStepF("BlockBitmap", "free", bmpIdx, -1, "truncate", name, 35 + i * 10,
                "Freeing data block #%d in Block Bitmap...", blk);
        }
        fireStepF("InodeTable", "update", inodeNum - 1, 0, "truncate", name, 75,
            "Clearing inode #%d buffer: size \u2192 0, data buffer zeroed...", inodeNum);
    }

    int result = truncate_File(name);
    if (result >= 0) {
        fireStep("SuperBlock", "update", -1, -1, "truncate", name, 85,
            "Updating Super Block free block count...");
        sync_superblock();
        fireStep("DiskSync", "sync", -1, -1, "truncate", name, 95,
            "Synchronizing cleared inode to virtual disk...");
        int fd = GetFDFromName(name);
        if (fd >= 0 && UFDTArr[fd].ptrfiletable) {
            sync_inode(UFDTArr[fd].ptrfiletable->ptrinode->InodeNumber);
        }
    }
    fireStepF("", "complete", -1, -1, "truncate", name, 100, "File '%s' truncated to 0 bytes.", name);
    if (g_postOpHook) g_postOpHook("truncate", name, 0, 0, result);
    return result;
}

int W_chmod_File(char *name, int permission)
{
    if (g_preOpHook) g_preOpHook("chmod", name, permission, 0);

    fireStepF("Directory", "find", -1, -1, "chmod", name, 20,
        "Finding '%s' in directory entries to change permissions...", name);

    PINODE node = FindFile(name);
    if (node) {
        int oldPerm = node->permission;
        int inodeNum = node->InodeNumber;
        fireStepF("InodeTable", "read", inodeNum - 1, oldPerm, "chmod", name, 40,
            "Reading inode #%d: current permission=%d (old), changing to %d (new)...", inodeNum, oldPerm, permission);
        node->permission = permission;
        fireStepF("InodeTable", "update", inodeNum - 1, permission, "chmod", name, 65,
            "Updated inode #%d permission field from %d to %d.", inodeNum, oldPerm, permission);
        sync_inode(node->InodeNumber);
        fireStep("DiskSync", "sync", -1, -1, "chmod", name, 85,
            "Synchronizing updated inode to virtual disk...");
    }
    fireStepF("", "complete", -1, -1, "chmod", name, 100, "Permissions changed for '%s'.", name);
    if (g_postOpHook) g_postOpHook("chmod", name, permission, 0, node ? 0 : -1);
    return node ? 0 : -1;
}

void W_ls_file()
{
    ls_file();
}

int W_dir_create(const char *name)
{
    if (g_preOpHook) g_preOpHook("mkdir", name, 0, 0);

    fireStep("SuperBlock", "check", -1, -1, "mkdir", name, 5,
        "Checking Super Block for available inode for new directory...");
    fireStep("InodeBitmap", "scan", -1, -1, "mkdir", name, 15,
        "Scanning Inode Bitmap for a free inode slot...");

    int result = dir_create(name);
    if (result >= 0) {
        int childInode = dir_find_entry(dir_get_cwd_inode(), name);
        if (childInode > 0) {
            fireStepF("InodeBitmap", "allocate", childInode - 1, -1, "mkdir", name, 30,
                "Allocating inode #%d for new directory in Inode Bitmap...", childInode);
            fireStepF("InodeTable", "update", childInode - 1, -1, "mkdir", name, 45,
                "Initializing directory inode #%d: type=DIRECTORY, links=2, buffer allocated...", childInode);
            fireStep("Directory", "init", childInode - 1, -1, "mkdir", name, 60,
                "Creating '.' (self) and '..' (parent) entries in new directory buffer...");
            fireStepF("Directory", "add", childInode - 1, dir_get_cwd_inode(), "mkdir", name, 75,
                "Adding entry '%s' \u2192 inode #%d to parent directory.", name, childInode);

            sync_inode(dir_get_cwd_inode());
            sync_inode(childInode);
            sync_superblock();

            fireStep("DiskSync", "sync", -1, -1, "mkdir", name, 90,
                "Synchronizing directory inodes and Super Block to virtual disk...");
        }
    }
    fireStepF("", "complete", -1, -1, "mkdir", name, 100,
        "Directory '%s' created with '.' and '..' entries.", name);
    if (g_postOpHook) g_postOpHook("mkdir", name, 0, 0, result);
    return result;
}

int W_dir_remove(const char *name)
{
    if (g_preOpHook) g_preOpHook("rmdir", name, 0, 0);

    fireStepF("Directory", "scan", -1, -1, "rmdir", name, 10,
        "Locating directory '%s' in parent directory entries...", name);

    int childInode = dir_find_entry(dir_get_cwd_inode(), name);
    if (childInode > 0) {
        fireStep("Directory", "check", childInode, -1, "rmdir", name, 25,
            "Verifying directory is empty (only '.' and '..' present)...");

        if (dir_is_empty(childInode)) {
            fireStepF("InodeTable", "read", childInode - 1, -1, "rmdir", name, 40,
                "Reading inode #%d: releasing directory buffer and metadata...", childInode);
            fireStepF("InodeBitmap", "free", childInode - 1, -1, "rmdir", name, 55,
                "Marking inode #%d as free in Inode Bitmap...", childInode);
            fireStepF("Directory", "remove", -1, -1, "rmdir", name, 70,
                "Removing directory entry for '%s' from parent...", name);
            fireStep("SuperBlock", "update", -1, -1, "rmdir", name, 82,
                "Incrementing free inode count in Super Block...");
        }
    }

    int result = dir_remove(name);
    if (result >= 0) {
        sync_inode(dir_get_cwd_inode());
        sync_superblock();
        fireStep("DiskSync", "sync", -1, -1, "rmdir", name, 95,
            "Synchronizing directory and Super Block to virtual disk...");
    }
    fireStepF("", "complete", -1, -1, "rmdir", name, 100, "Directory '%s' removed.", name);
    if (g_postOpHook) g_postOpHook("rmdir", name, 0, 0, result);
    return result;
}

int W_dir_change(const char *path)
{
    if (g_preOpHook) g_preOpHook("cd", path, 0, 0);

    fireStepF("Directory", "resolve", -1, -1, "cd", path, 30,
        "Resolving path '%s' by traversing directory entries...", path);

    int result = dir_change(path);
    if (result >= 0) {
        fireStepF("Directory", "cwd", dir_get_cwd_inode(), -1, "cd", path, 70,
            "Changed current working directory to inode #%d.", dir_get_cwd_inode());
    }
    fireStepF("", "complete", -1, -1, "cd", path, 100, "Current directory: %s", path);
    if (g_postOpHook) g_postOpHook("cd", path, 0, 0, result);
    return result;
}
