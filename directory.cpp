#include "directory.h"
#include "persistence.h"
#include <string.h>

static int g_cwdInode = ROOT_INODE;
static char g_cwdPath[MAX_PATH] = "/";

int dir_get_cwd_inode()
{
    return g_cwdInode;
}

const char* dir_get_cwd_path()
{
    return g_cwdPath;
}

int dir_set_cwd_inode(int inode)
{
    if (inode < 1 || inode > MAXINODE) return -1;
    PINODE temp = head;
    while (temp != NULL) {
        if (temp->InodeNumber == inode && temp->FileType == DIRECTORY) {
            g_cwdInode = inode;
            return 0;
        }
        temp = temp->next;
    }
    return -1;
}

static PINODE find_inode_by_number(int inodeNum)
{
    PINODE temp = head;
    while (temp != NULL) {
        if (temp->InodeNumber == inodeNum) return temp;
        temp = temp->next;
    }
    return NULL;
}

int dir_resolve_path(const char *path, int *outInode)
{
    if (!path || !outInode) return -1;

    if (strcmp(path, "/") == 0) {
        *outInode = ROOT_INODE;
        return 0;
    }
    if (strcmp(path, ".") == 0) {
        *outInode = g_cwdInode;
        return 0;
    }

    char localPath[MAX_PATH];
    strncpy(localPath, path, MAX_PATH - 1);
    localPath[MAX_PATH - 1] = '\0';

    int startInode = g_cwdInode;
    if (localPath[0] == '/') {
        startInode = ROOT_INODE;
    }

    char *token = strtok(localPath, "/");
    int currentInode = startInode;

    while (token != NULL) {
        if (strcmp(token, ".") == 0) {
            token = strtok(NULL, "/");
            continue;
        }
        if (strcmp(token, "..") == 0) {
            PINODE curNode = find_inode_by_number(currentInode);
            if (!curNode || !curNode->Buffer) return -1;
            DirEntry *entries = (DirEntry *)curNode->Buffer;
            int numEntries = curNode->FileActualSize / sizeof(DirEntry);
            int found = 0;
            for (int i = 0; i < numEntries && i < MAX_DIR_ENTRIES; i++) {
                if (entries[i].d_inode > 0 && strcmp(entries[i].d_name, "..") == 0) {
                    currentInode = entries[i].d_inode;
                    found = 1;
                    break;
                }
            }
            if (!found) return -1;
            token = strtok(NULL, "/");
            continue;
        }

        PINODE dirNode = find_inode_by_number(currentInode);
        if (!dirNode || !dirNode->Buffer) return -1;

        DirEntry *entries = (DirEntry *)dirNode->Buffer;
        int numEntries = dirNode->FileActualSize / sizeof(DirEntry);
        int found = 0;

        for (int i = 0; i < numEntries && i < MAX_DIR_ENTRIES; i++) {
            if (entries[i].d_inode > 0 && strcmp(entries[i].d_name, token) == 0) {
                currentInode = entries[i].d_inode;
                found = 1;
                break;
            }
        }
        if (!found) return -1;

        token = strtok(NULL, "/");
    }

    *outInode = currentInode;
    return 0;
}

int dir_resolve_parent(const char *path, char *outName)
{
    if (!path || !outName) return -1;

    char localPath[MAX_PATH];
    strncpy(localPath, path, MAX_PATH - 1);
    localPath[MAX_PATH - 1] = '\0';

    char *lastSlash = strrchr(localPath, '/');
    if (!lastSlash) {
        strncpy(outName, localPath, 59);
        outName[59] = '\0';
        return g_cwdInode;
    }

    if (lastSlash == localPath) {
        strncpy(outName, localPath + 1, 59);
        outName[59] = '\0';
        return ROOT_INODE;
    }

    strncpy(outName, lastSlash + 1, 59);
    outName[59] = '\0';
    *lastSlash = '\0';

    int parentInode = 0;
    if (dir_resolve_path(localPath, &parentInode) != 0)
        return -1;
    return parentInode;
}

static int find_parent_inode(int childInode)
{
    if (childInode == ROOT_INODE) return ROOT_INODE;
    PINODE temp = head;
    while (temp != NULL) {
        if (temp->FileType == DIRECTORY && temp->Buffer) {
            DirEntry *entries = (DirEntry *)temp->Buffer;
            int numEntries = temp->FileActualSize / sizeof(DirEntry);
            for (int i = 0; i < numEntries && i < MAX_DIR_ENTRIES; i++) {
                if (entries[i].d_inode == childInode) {
                    return temp->InodeNumber;
                }
            }
        }
        temp = temp->next;
    }
    return ROOT_INODE;
}

static int ensure_dir_buffer(PINODE dirNode)
{
    if (!dirNode->Buffer) {
        dirNode->Buffer = (char *)calloc(1, MAXFILESIZE);
        if (!dirNode->Buffer) return -1;

        DirEntry dot;
        dot.d_inode = dirNode->InodeNumber;
        strcpy(dot.d_name, ".");
        dot.d_type = DIRECTORY;

        DirEntry dotdot;
        int parentInode = find_parent_inode(dirNode->InodeNumber);
        dotdot.d_inode = parentInode;
        strcpy(dotdot.d_name, "..");
        dotdot.d_type = DIRECTORY;

        memcpy(dirNode->Buffer, &dot, sizeof(DirEntry));
        memcpy(dirNode->Buffer + sizeof(DirEntry), &dotdot, sizeof(DirEntry));
        dirNode->FileActualSize = 2 * sizeof(DirEntry);
    }
    return 0;
}

int dir_create(const char *name)
{
    if (!name || strlen(name) == 0) return -1;

    char entryName[60];
    int parentInode;

    if (strchr(name, '/') != NULL) {
        parentInode = dir_resolve_parent(name, entryName);
    } else {
        strncpy(entryName, name, 59);
        entryName[59] = '\0';
        parentInode = g_cwdInode;
    }

    if (parentInode < 1) return -1;

    PINODE parentNode = find_inode_by_number(parentInode);
    if (!parentNode || parentNode->FileType != DIRECTORY) return -1;

    if (ensure_dir_buffer(parentNode) != 0) return -1;

    DirEntry *entries = (DirEntry *)parentNode->Buffer;
    int numEntries = parentNode->FileActualSize / sizeof(DirEntry);
    for (int i = 0; i < numEntries && i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].d_inode > 0 && strcmp(entries[i].d_name, entryName) == 0)
            return -3;
    }

    int fd = CreateFile(entryName, 3);
    if (fd < 0) return fd;

    PINODE newDirNode = UFDTArr[fd].ptrfiletable->ptrinode;
    newDirNode->FileType = DIRECTORY;

    CloseFileByFD(fd);

    if (ensure_dir_buffer(newDirNode) != 0) return -1;

    DirEntry *newEntries = (DirEntry *)newDirNode->Buffer;
    newEntries[0].d_inode = newDirNode->InodeNumber;
    strcpy(newEntries[0].d_name, ".");
    newEntries[0].d_type = DIRECTORY;

    newEntries[1].d_inode = parentInode;
    strcpy(newEntries[1].d_name, "..");
    newEntries[1].d_type = DIRECTORY;

    newDirNode->FileActualSize = 2 * sizeof(DirEntry);
    newDirNode->LinkCount = 1;

    if (numEntries < MAX_DIR_ENTRIES) {
        entries[numEntries].d_inode = newDirNode->InodeNumber;
        strncpy(entries[numEntries].d_name, entryName, 58);
        entries[numEntries].d_name[58] = '\0';
        entries[numEntries].d_type = DIRECTORY;
        parentNode->FileActualSize = (numEntries + 1) * sizeof(DirEntry);
    }

    return 0;
}

int dir_remove(const char *name)
{
    if (!name) return -1;

    char entryName[60];
    int parentInode;

    if (strchr(name, '/') != NULL) {
        parentInode = dir_resolve_parent(name, entryName);
    } else {
        strncpy(entryName, name, 59);
        entryName[59] = '\0';
        parentInode = g_cwdInode;
    }

    if (parentInode < 1) return -1;

    PINODE parentNode = find_inode_by_number(parentInode);
    if (!parentNode || !parentNode->Buffer) return -1;

    DirEntry *entries = (DirEntry *)parentNode->Buffer;
    int numEntries = parentNode->FileActualSize / sizeof(DirEntry);

    int targetInode = -1;
    int targetIdx = -1;
    for (int i = 0; i < numEntries && i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].d_inode > 0 && strcmp(entries[i].d_name, entryName) == 0) {
            targetInode = entries[i].d_inode;
            targetIdx = i;
            break;
        }
    }

    if (targetInode < 1 || targetIdx < 0) return -1;

    PINODE targetNode = find_inode_by_number(targetInode);
    if (!targetNode || targetNode->FileType != DIRECTORY) return -2;

    if (!dir_is_empty(targetInode)) return -3;

    if (targetInode == g_cwdInode) return -4;

    entries[targetIdx].d_inode = 0;
    memset(entries[targetIdx].d_name, 0, 59);
    entries[targetIdx].d_type = 0;

    targetNode->FileType = 0;
    if (targetNode->Buffer) {
        free(targetNode->Buffer);
        targetNode->Buffer = NULL;
    }
    targetNode->FileActualSize = 0;
    targetNode->LinkCount = 0;
    memset(targetNode->FileName, 0, 50);

    SUPERBLOCKobj.FreeInode++;

    sync_inode(parentInode);
    sync_inode(targetInode);
    sync_superblock();

    return 0;
}

int dir_change(const char *path)
{
    if (!path) return -1;

    int targetInode;

    if (strcmp(path, "/") == 0) {
        targetInode = ROOT_INODE;
    } else if (strcmp(path, "..") == 0) {
        PINODE curNode = find_inode_by_number(g_cwdInode);
        if (!curNode || !curNode->Buffer) return -1;
        DirEntry *entries = (DirEntry *)curNode->Buffer;
        int numEntries = curNode->FileActualSize / sizeof(DirEntry);
        targetInode = -1;
        for (int i = 0; i < numEntries && i < MAX_DIR_ENTRIES; i++) {
            if (entries[i].d_inode > 0 && strcmp(entries[i].d_name, "..") == 0) {
                targetInode = entries[i].d_inode;
                break;
            }
        }
        if (targetInode < 1) return -1;
    } else if (strcmp(path, ".") == 0) {
        targetInode = g_cwdInode;
    } else {
        if (dir_resolve_path(path, &targetInode) != 0) return -1;
    }

    PINODE node = find_inode_by_number(targetInode);
    if (!node || node->FileType != DIRECTORY) return -2;

    g_cwdInode = targetInode;

    if (strcmp(path, "/") == 0) {
        strcpy(g_cwdPath, "/");
    } else if (path[0] == '/') {
        strncpy(g_cwdPath, path, MAX_PATH - 1);
        g_cwdPath[MAX_PATH - 1] = '\0';
    } else if (strcmp(path, "..") == 0) {
        char *lastSlash = strrchr(g_cwdPath, '/');
        if (lastSlash == g_cwdPath) {
            strcpy(g_cwdPath, "/");
        } else if (lastSlash) {
            *lastSlash = '\0';
        }
    } else {
        if (strcmp(g_cwdPath, "/") != 0)
            strncat(g_cwdPath, "/", MAX_PATH - strlen(g_cwdPath) - 1);
        strncat(g_cwdPath, path, MAX_PATH - strlen(g_cwdPath) - 1);
    }

    return 0;
}

int dir_list(DirListing *listing)
{
    return dir_list_at(g_cwdInode, listing);
}

int dir_list_at(int dirInode, DirListing *listing)
{
    if (!listing) return -1;
    listing->count = 0;

    PINODE dirNode = find_inode_by_number(dirInode);
    if (!dirNode || !dirNode->Buffer) return -1;

    DirEntry *entries = (DirEntry *)dirNode->Buffer;
    int numEntries = dirNode->FileActualSize / sizeof(DirEntry);

    for (int i = 0; i < numEntries && i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].d_inode == 0) continue;
        if (strcmp(entries[i].d_name, ".") == 0 || strcmp(entries[i].d_name, "..") == 0) continue;

        DirListingEntry *le = &listing->entries[listing->count];
        strncpy(le->name, entries[i].d_name, 59);
        le->name[59] = '\0';
        le->inode = entries[i].d_inode;
        le->type = entries[i].d_type;

        PINODE fileNode = find_inode_by_number(entries[i].d_inode);
        if (fileNode) {
            le->size = fileNode->FileActualSize;
            le->permission = fileNode->permission;
        } else {
            le->size = 0;
            le->permission = 0;
        }

        listing->count++;
    }

    return 0;
}

int dir_add_entry(int dirInode, int entryInode, const char *name, char type)
{
    PINODE dirNode = find_inode_by_number(dirInode);
    if (!dirNode || !dirNode->Buffer) return -1;

    DirEntry *entries = (DirEntry *)dirNode->Buffer;
    int numEntries = dirNode->FileActualSize / sizeof(DirEntry);

    if (numEntries >= MAX_DIR_ENTRIES) return -2;

    entries[numEntries].d_inode = entryInode;
    strncpy(entries[numEntries].d_name, name, 58);
    entries[numEntries].d_name[58] = '\0';
    entries[numEntries].d_type = type;
    dirNode->FileActualSize = (numEntries + 1) * sizeof(DirEntry);

    return 0;
}

int dir_remove_entry(int dirInode, const char *name)
{
    PINODE dirNode = find_inode_by_number(dirInode);
    if (!dirNode || !dirNode->Buffer) return -1;

    DirEntry *entries = (DirEntry *)dirNode->Buffer;
    int numEntries = dirNode->FileActualSize / sizeof(DirEntry);

    for (int i = 0; i < numEntries && i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].d_inode > 0 && strcmp(entries[i].d_name, name) == 0) {
            entries[i].d_inode = 0;
            memset(entries[i].d_name, 0, 59);
            entries[i].d_type = 0;
            return 0;
        }
    }
    return -1;
}

int dir_find_entry(int dirInode, const char *name)
{
    PINODE dirNode = find_inode_by_number(dirInode);
    if (!dirNode || !dirNode->Buffer) return -1;

    DirEntry *entries = (DirEntry *)dirNode->Buffer;
    int numEntries = dirNode->FileActualSize / sizeof(DirEntry);

    for (int i = 0; i < numEntries && i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].d_inode > 0 && strcmp(entries[i].d_name, name) == 0) {
            return entries[i].d_inode;
        }
    }
    return -1;
}

int dir_is_empty(int dirInode)
{
    PINODE dirNode = find_inode_by_number(dirInode);
    if (!dirNode || !dirNode->Buffer) return 1;

    DirEntry *entries = (DirEntry *)dirNode->Buffer;
    int numEntries = dirNode->FileActualSize / sizeof(DirEntry);

    int realEntries = 0;
    for (int i = 0; i < numEntries && i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].d_inode > 0) {
            if (strcmp(entries[i].d_name, ".") != 0 &&
                strcmp(entries[i].d_name, "..") != 0) {
                realEntries++;
            }
        }
    }
    return (realEntries == 0);
}
