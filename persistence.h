#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "CVFS.h"

#define BLOCK_SIZE              1024
#define TOTAL_BLOCKS            512
#define SUPER_BLOCK_BLOCK       0
#define BLOCK_BITMAP_BLOCK      1
#define INODE_BITMAP_BLOCK      2
#define INODE_TABLE_START       3
#define INODE_TABLE_BLOCKS      7
#define INODE_TABLE_SIZE        (INODE_TABLE_BLOCKS * (BLOCK_SIZE / 128))
#define DATA_START_BLOCK        10
#define NUM_DATA_BLOCKS         (TOTAL_BLOCKS - DATA_START_BLOCK)
#define CVFS_MAGIC              0xEF53

#pragma pack(push, 1)
typedef struct disk_superblock
{
    int s_inodes_count;
    int s_free_inodes_count;
    int s_blocks_count;
    int s_free_blocks_count;
    int s_first_data_block;
    int s_block_size;
    int s_magic;
    char reserved[996];
}DiskSuperBlock;

typedef struct disk_inode
{
    char i_filename[48];
    int i_mode;
    int i_size;
    int i_links_count;
    int i_blocks_count;
    int i_block[4];
    char i_reserved[48];
}DiskInode;

typedef struct dir_entry
{
    int d_inode;
    char d_name[59];
    char d_type;
}DirEntry;
#pragma pack(pop)

#define DIR_ENTRIES_PER_BLOCK (BLOCK_SIZE / sizeof(DirEntry))

extern const char *g_diskPath;
extern unsigned char g_blockBitmap[];
extern unsigned char g_inodeBitmap[];
void set_bit(unsigned char *bitmap, int bit, int value);
int get_bit(unsigned char *bitmap, int bit);

int mount_virtual_disk(const char *diskPath);
void unmount_virtual_disk();

int format_disk(const char *diskPath);

int read_block(int blockNum, void *buffer);
int write_block(int blockNum, void *buffer);

int read_inode(int inodeNum, DiskInode *diskInode);
int write_inode(int inodeNum, DiskInode *diskInode);

int allocate_data_block();
void free_data_block(int blockNum);
int allocate_inode_bit();
void free_inode_bit(int inodeNum);

int sync_inode(int inodeNum);
int sync_superblock();

int get_free_blocks_count();
int get_free_inodes_count();
int is_block_free(int blockIdx);
int is_inode_free(int inodeNum);

#endif
