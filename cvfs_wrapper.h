#ifndef CVFS_WRAPPER_H
#define CVFS_WRAPPER_H

#include "CVFS.h"

#define CVFS_DISK_PATH "cvfs.img"

typedef void (*PreOpHook)(const char *opName, const char *param1, int param2, int param3);
typedef void (*PostOpHook)(const char *opName, const char *param1, int param2, int param3, int result);
typedef void (*StepHook)(const char *component, const char *action,
                         int targetIndex, int extraIndex,
                         const char *operation, const char *fileName,
                         int progressPercent, const char *explanation);

extern PreOpHook g_preOpHook;
extern PostOpHook g_postOpHook;
extern StepHook g_stepHook;

void CVFS_Init();
void CVFS_Shutdown();
int W_CreateFile(char *name, int permission);
int W_rm_File(char *name);
int W_ReadFile(int fd, char *arr, int isize);
int W_WriteFile(int fd, char *arr, int isize);
int W_OpenFile(char *name, int mode);
int W_CloseFileByName(char *name);
void W_CloseAllFile();
int W_LseekFile(int fd, int size, int from);
int W_truncate_File(char *name);
void W_ls_file();
int W_chmod_File(char *name, int permission);

int W_dir_create(const char *name);
int W_dir_remove(const char *name);
int W_dir_change(const char *path);

#endif
