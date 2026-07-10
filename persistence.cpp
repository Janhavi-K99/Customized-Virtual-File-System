#include "persistence.h"
#include "directory.h"
#include <string.h>

static FILE *g_diskFile = NULL;
const char *g_diskPath = NULL;

unsigned char g_blockBitmap[BLOCK_SIZE] = {0};
unsigned char g_inodeBitmap[BLOCK_SIZE] = {0};
static DiskSuperBlock g_superBlockBuf;

void set_bit(unsigned char *bitmap, int bit, int value)
{
    int byteIdx = bit / 8;
    int bitIdx = bit % 8;
    if (byteIdx >= BLOCK_SIZE) return;
    if (value)
        bitmap[byteIdx] |= (1 << bitIdx);
    else
        bitmap[byteIdx] &= ~(1 << bitIdx);
}

int get_bit(unsigned char *bitmap, int bit)
{
    int byteIdx = bit / 8;
    int bitIdx = bit % 8;
    if (byteIdx >= BLOCK_SIZE) return 0;
    return (bitmap[byteIdx] >> bitIdx) & 1;
}

int read_block(int blockNum, void *buffer)
{
    if (!g_diskFile || blockNum < 0 || blockNum >= TOTAL_BLOCKS) return -1;
    fseek(g_diskFile, blockNum * BLOCK_SIZE, SEEK_SET);
    return (fread(buffer, 1, BLOCK_SIZE, g_diskFile) == BLOCK_SIZE) ? 0 : -1;
}

int write_block(int blockNum, void *buffer)
{
    if (!g_diskFile || blockNum < 0 || blockNum >= TOTAL_BLOCKS) return -1;
    fseek(g_diskFile, blockNum * BLOCK_SIZE, SEEK_SET);
    return (fwrite(buffer, 1, BLOCK_SIZE, g_diskFile) == BLOCK_SIZE) ? 0 : -1;
}

int read_inode(int inodeNum, DiskInode *diskInode)
{
    if (inodeNum < 1 || inodeNum > INODE_TABLE_SIZE) return -1;
    int entryIdx = inodeNum - 1;
    int blockNum = INODE_TABLE_START + (entryIdx / (BLOCK_SIZE / sizeof(DiskInode)));
    int offsetInBlock = (entryIdx % (BLOCK_SIZE / sizeof(DiskInode))) * sizeof(DiskInode);

    unsigned char blockBuf[BLOCK_SIZE];
    if (read_block(blockNum, blockBuf) != 0) return -1;
    memcpy(diskInode, blockBuf + offsetInBlock, sizeof(DiskInode));
    return 0;
}

int write_inode(int inodeNum, DiskInode *diskInode)
{
    if (inodeNum < 1 || inodeNum > INODE_TABLE_SIZE) return -1;
    int entryIdx = inodeNum - 1;
    int blockNum = INODE_TABLE_START + (entryIdx / (BLOCK_SIZE / sizeof(DiskInode)));
    int offsetInBlock = (entryIdx % (BLOCK_SIZE / sizeof(DiskInode))) * sizeof(DiskInode);

    unsigned char blockBuf[BLOCK_SIZE];
    if (read_block(blockNum, blockBuf) != 0) memset(blockBuf, 0, BLOCK_SIZE);
    memcpy(blockBuf + offsetInBlock, diskInode, sizeof(DiskInode));
    return write_block(blockNum, blockBuf);
}

int allocate_data_block()
{
    for (int i = 0; i < NUM_DATA_BLOCKS; i++) {
        if (get_bit(g_blockBitmap, i)) {
            set_bit(g_blockBitmap, i, 0);
            g_superBlockBuf.s_free_blocks_count--;
            return DATA_START_BLOCK + i;
        }
    }
    return -1;
}

void free_data_block(int blockNum)
{
    int idx = blockNum - DATA_START_BLOCK;
    if (idx >= 0 && idx < NUM_DATA_BLOCKS) {
        set_bit(g_blockBitmap, idx, 1);
        g_superBlockBuf.s_free_blocks_count++;
    }
}

int allocate_inode_bit()
{
    for (int i = 0; i < MAXINODE; i++) {
        if (get_bit(g_inodeBitmap, i)) {
            set_bit(g_inodeBitmap, i, 0);
            g_superBlockBuf.s_free_inodes_count--;
            return i + 1;
        }
    }
    return -1;
}

void free_inode_bit(int inodeNum)
{
    if (inodeNum >= 1 && inodeNum <= MAXINODE) {
        set_bit(g_inodeBitmap, inodeNum - 1, 1);
        g_superBlockBuf.s_free_inodes_count++;
    }
}

int get_free_blocks_count()
{
    return g_superBlockBuf.s_free_blocks_count;
}

int get_free_inodes_count()
{
    return g_superBlockBuf.s_free_inodes_count;
}

int is_block_free(int blockIdx)
{
    if (blockIdx < 0 || blockIdx >= NUM_DATA_BLOCKS) return 0;
    return get_bit(g_blockBitmap, blockIdx);
}

int is_inode_free(int inodeNum)
{
    if (inodeNum < 1 || inodeNum > MAXINODE) return 0;
    return get_bit(g_inodeBitmap, inodeNum - 1);
}

static void disk_inode_to_incore(DiskInode *diskInode, PINODE node)
{
    if (!diskInode || !node) return;
    strncpy(node->FileName, diskInode->i_filename, 49);
    node->FileName[49] = '\0';
    node->FileType = diskInode->i_mode & 0x03;
    node->permission = (diskInode->i_mode >> 2) & 0x03;
    node->FileActualSize = diskInode->i_size;
    node->LinkCount = diskInode->i_links_count;
    node->FileSize = MAXFILESIZE;
    node->ReferenceCount = 0;
    node->Buffer = (char *)calloc(1, MAXFILESIZE);
    if (node->Buffer && node->FileType != 0) {
        for (int i = 0; i < diskInode->i_blocks_count && i < 4; i++) {
            if (diskInode->i_block[i] >= DATA_START_BLOCK) {
                read_block(diskInode->i_block[i], node->Buffer + i * BLOCK_SIZE);
            }
        }
    }
}

static void incore_to_disk_inode(PINODE node, DiskInode *diskInode)
{
    if (!node || !diskInode) return;
    memset(diskInode, 0, sizeof(DiskInode));
    strncpy(diskInode->i_filename, node->FileName, 47);
    diskInode->i_filename[47] = '\0';
    diskInode->i_mode = (node->FileType & 0x03) | ((node->permission & 0x03) << 2);
    diskInode->i_size = node->FileActualSize;
    diskInode->i_links_count = node->LinkCount;
    diskInode->i_blocks_count = 0;

    if (node->Buffer && node->FileActualSize > 0) {
        int totalSize = node->FileActualSize;
        int blockIdx = 0;
        while (totalSize > 0 && blockIdx < 4) {
            int blk = allocate_data_block();
            if (blk < 0) break;
            diskInode->i_block[blockIdx] = blk;
            int writeSize = (totalSize > BLOCK_SIZE) ? BLOCK_SIZE : totalSize;
            write_block(blk, node->Buffer + blockIdx * BLOCK_SIZE);
            totalSize -= writeSize;
            diskInode->i_blocks_count++;
            blockIdx++;
        }
    }
}

int sync_inode(int inodeNum)
{
    PINODE temp = head;
    while (temp != NULL) {
        if (temp->InodeNumber == inodeNum) break;
        temp = temp->next;
    }
    if (!temp) return -1;

    DiskInode diskInode;
    incore_to_disk_inode(temp, &diskInode);
    return write_inode(inodeNum, &diskInode);
}

int sync_superblock()
{
    g_superBlockBuf.s_inodes_count = SUPERBLOCKobj.TotalInodes;
    g_superBlockBuf.s_free_inodes_count = SUPERBLOCKobj.FreeInode;

    if (write_block(SUPER_BLOCK_BLOCK, &g_superBlockBuf) != 0) return -1;
    if (write_block(BLOCK_BITMAP_BLOCK, g_blockBitmap) != 0) return -1;
    if (write_block(INODE_BITMAP_BLOCK, g_inodeBitmap) != 0) return -1;
    return 0;
}

static void sync_all_inodes()
{
    PINODE temp = head;
    while (temp != NULL) {
        if (temp->FileType != 0) {
            sync_inode(temp->InodeNumber);
        }
        temp = temp->next;
    }
}

int format_disk(const char *diskPath)
{
    FILE *fp = fopen(diskPath, "wb");
    if (!fp) return -1;

    unsigned char zeroBlock[BLOCK_SIZE];
    memset(zeroBlock, 0, BLOCK_SIZE);

    for (int i = 0; i < TOTAL_BLOCKS; i++) {
        fwrite(zeroBlock, 1, BLOCK_SIZE, fp);
    }
    fclose(fp);

    fp = fopen(diskPath, "rb+");
    if (!fp) return -1;

    DiskSuperBlock sb;
    memset(&sb, 0, sizeof(sb));
    sb.s_inodes_count = MAXINODE;
    sb.s_free_inodes_count = MAXINODE;
    sb.s_blocks_count = NUM_DATA_BLOCKS;
    sb.s_free_blocks_count = NUM_DATA_BLOCKS;
    sb.s_first_data_block = DATA_START_BLOCK;
    sb.s_block_size = BLOCK_SIZE;
    sb.s_magic = CVFS_MAGIC;

    fseek(fp, 0, SEEK_SET);
    fwrite(&sb, sizeof(sb), 1, fp);

    unsigned char bitmap[BLOCK_SIZE];
    memset(bitmap, 0xFF, BLOCK_SIZE);
    fseek(fp, BLOCK_BITMAP_BLOCK * BLOCK_SIZE, SEEK_SET);
    fwrite(bitmap, 1, BLOCK_SIZE, fp);
    fseek(fp, INODE_BITMAP_BLOCK * BLOCK_SIZE, SEEK_SET);
    fwrite(bitmap, 1, BLOCK_SIZE, fp);

    g_diskFile = fp;

    DiskInode rootInode;
    memset(&rootInode, 0, sizeof(DiskInode));
    strcpy(rootInode.i_filename, "/");
    rootInode.i_mode = (DIRECTORY & 0x03) | ((3 & 0x03) << 2);
    rootInode.i_size = 0;
    rootInode.i_links_count = 1;
    rootInode.i_blocks_count = 0;
    write_inode(1, &rootInode);

    set_bit(bitmap, 0, 0);
    fseek(fp, INODE_BITMAP_BLOCK * BLOCK_SIZE, SEEK_SET);
    fwrite(bitmap, 1, BLOCK_SIZE, fp);

    fclose(fp);
    g_diskFile = NULL;

    g_superBlockBuf = sb;
    memset(g_blockBitmap, 0xFF, BLOCK_SIZE);
    memset(g_inodeBitmap, 0xFF, BLOCK_SIZE);
    set_bit(g_inodeBitmap, 0, 0);

    return 0;
}

int mount_virtual_disk(const char *diskPath)
{
    g_diskPath = diskPath;

    FILE *fp = fopen(diskPath, "rb+");
    if (!fp) {
        if (format_disk(diskPath) != 0) return -1;
        fp = fopen(diskPath, "rb+");
        if (!fp) return -1;
    }

    g_diskFile = fp;

    if (read_block(SUPER_BLOCK_BLOCK, &g_superBlockBuf) != 0) return -1;
    if (g_superBlockBuf.s_magic != CVFS_MAGIC) {
        fclose(g_diskFile);
        g_diskFile = NULL;
        if (format_disk(diskPath) != 0) return -1;
        fp = fopen(diskPath, "rb+");
        if (!fp) return -1;
        g_diskFile = fp;
        read_block(SUPER_BLOCK_BLOCK, &g_superBlockBuf);
    }

    read_block(BLOCK_BITMAP_BLOCK, g_blockBitmap);
    read_block(INODE_BITMAP_BLOCK, g_inodeBitmap);

    SUPERBLOCKobj.TotalInodes = g_superBlockBuf.s_inodes_count;
    SUPERBLOCKobj.FreeInode = g_superBlockBuf.s_free_inodes_count;

    for (int i = 1; i <= MAXINODE; i++) {
        PINODE node = head;
        while (node != NULL && node->InodeNumber != i)
            node = node->next;
        if (!node) continue;

        if (!get_bit(g_inodeBitmap, i - 1)) {
            DiskInode diskInode;
            if (read_inode(i, &diskInode) == 0) {
                disk_inode_to_incore(&diskInode, node);
            }
        } else {
            node->FileType = 0;
            node->LinkCount = 0;
            node->ReferenceCount = 0;
            node->Buffer = NULL;
        }
    }

    PINODE rootNode = head;
    while (rootNode && rootNode->InodeNumber != ROOT_INODE)
        rootNode = rootNode->next;
    if (rootNode && (!rootNode->Buffer || rootNode->FileActualSize == 0 || rootNode->FileType != DIRECTORY)) {
        rootNode->FileType = DIRECTORY;
        if (!rootNode->Buffer)
            rootNode->Buffer = (char *)calloc(1, MAXFILESIZE);
        DirEntry dot;
        dot.d_inode = ROOT_INODE;
        strcpy(dot.d_name, ".");
        dot.d_type = DIRECTORY;
        DirEntry dotdot;
        dotdot.d_inode = ROOT_INODE;
        strcpy(dotdot.d_name, "..");
        dotdot.d_type = DIRECTORY;
        memcpy(rootNode->Buffer, &dot, sizeof(DirEntry));
        memcpy(rootNode->Buffer + sizeof(DirEntry), &dotdot, sizeof(DirEntry));
        rootNode->FileActualSize = 2 * sizeof(DirEntry);
    }

    return 0;
}

void unmount_virtual_disk()
{
    if (!g_diskPath) return;

    if (!g_diskFile) {
        g_diskFile = fopen(g_diskPath, "rb+");
        if (!g_diskFile) return;
    }

    g_superBlockBuf.s_free_inodes_count = SUPERBLOCKobj.FreeInode;
    g_superBlockBuf.s_inodes_count = SUPERBLOCKobj.TotalInodes;

    sync_all_inodes();
    sync_superblock();

    fclose(g_diskFile);
    g_diskFile = NULL;
}
