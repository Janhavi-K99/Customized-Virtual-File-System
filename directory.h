#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "CVFS.h"

#define ROOT_INODE 1
#ifndef MAX_PATH
#define MAX_PATH 256
#endif
#define MAX_DIR_ENTRIES 64

typedef struct dir_listing_entry
{
    char name[60];
    int inode;
    int type;
    int size;
    int permission;
}DirListingEntry;

typedef struct dir_listing
{
    DirListingEntry entries[MAX_DIR_ENTRIES];
    int count;
}DirListing;

int dir_get_cwd_inode();
const char* dir_get_cwd_path();
int dir_set_cwd_inode(int inode);

int dir_resolve_path(const char *path, int *outInode);
int dir_resolve_parent(const char *path, char *outName);

int dir_create(const char *name);
int dir_remove(const char *name);
int dir_change(const char *path);
int dir_make_path(const char *name, char *fullPath);

int dir_list(DirListing *listing);
int dir_list_at(int dirInode, DirListing *listing);

int dir_add_entry(int dirInode, int entryInode, const char *name, char type);
int dir_remove_entry(int dirInode, const char *name);
int dir_find_entry(int dirInode, const char *name);

int dir_is_empty(int dirInode);

#endif
