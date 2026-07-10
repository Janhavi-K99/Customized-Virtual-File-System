#ifndef CVFS_H
#define CVFS_H

#define _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<iostream>

#define MAXINODE 50

#define READ 1
#define WRITE 2

#define MAXFILESIZE 2048

#define REGULAR 1
#define SPECIAL 2
#define DIRECTORY 3

#define START 0
#define CURRENT 1
#define END 2

typedef struct superblock
{
    int TotalInodes;
    int FreeInode;
}SUPERBLOCK, *PSUPERBLOCK;

typedef struct inode
{
    char FileName[50];
    int InodeNumber;
    int FileSize;
    int FileActualSize;
    int FileType;
    char *Buffer;
    int LinkCount;
    int ReferenceCount;
    int permission;
    struct inode *next;
}INODE, *PINODE, **PPINODE;

typedef struct filetable
{
    int readoffset;
    int writeoffset;
    int count;
    int mode;
    PINODE ptrinode;
}FILETABLE, *PFILETABLE;

typedef struct ufdt
{
    PFILETABLE ptrfiletable;
}UFDT;

extern UFDT UFDTArr[50];
extern SUPERBLOCK SUPERBLOCKobj;
extern PINODE head;

void man(char *name);
void DisplayHelp();
int GetFDFromName(char *name);
PINODE Get_Inode(char * name);
void CreateDILB();
void InitialiseSuperBlock();
int CreateFile(char *name, int permission);
int rm_File(char * name);
int ReadFile(int fd, char *arr, int isize);
int WriteFile(int fd, char *arr, int isize);
int OpenFile(char *name, int mode);
void CloseFileByFD(int fd);
int CloseFileByName(char *name);
void CloseAllFile();
int LseekFile(int fd, int size, int from);
void ls_file();
int fstat_file(int fd);
int stat_file(char *name);
int truncate_File(char *name);
PINODE FindFile(const char *name);

#endif
