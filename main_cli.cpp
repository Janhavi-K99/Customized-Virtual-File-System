#include "cvfs_wrapper.h"
#include "directory.h"
#include <string.h>
#include <stdlib.h>

int main()
{
    char *ptr = NULL;
    int ret = 0, fd = 0, count = 0;
    char command[4][80], str[80], arr[1024];

    CVFS_Init();

    while(1)
    {
        fflush(stdin);
        strcpy(str, "");

        printf("\nCVFS [%s] : >", dir_get_cwd_path());

        fgets(str,80,stdin);

        count = sscanf(str,"%s %s %s %s",command[0],command[1],command[2],command[3]);

        if(count == 1)
        {
            if(strcmp(command[0],"ls") == 0)
            {
                DirListing listing;
                dir_list(&listing);
                printf("\nName\t\tInode\tType\tSize\tPerm\n");
                printf("---------------------------------------------------------\n");
                for (int i = 0; i < listing.count; i++) {
                    printf("%s\t\t%d\t%c\t%d\t%d\n",
                           listing.entries[i].name,
                           listing.entries[i].inode,
                           listing.entries[i].type == DIRECTORY ? 'D' : 'F',
                           listing.entries[i].size,
                           listing.entries[i].permission);
                }
                printf("-----------------------------------------------------------\n");
            }
            else if(strcmp(command[0],"closeall") == 0)
            {
                W_CloseAllFile();
                printf("All files closed successfully\n");
                continue;
            }
            else if(strcmp(command[0],"clear") == 0)
            {
                system("cls");
                continue;
            }
            else if(strcmp(command[0],"help") == 0)
            {
                DisplayHelp();
                continue;
            }
            else if(strcmp(command[0],"exit") == 0)
            {
                printf("Terminating CVFS...\n");
                break;
            }
            else if(strcmp(command[0],"pwd") == 0)
            {
                printf("%s\n", dir_get_cwd_path());
            }
            else
            {
                printf("\nERROR : Command not found !!!\n");
                continue;
            }
        }
        else if(count == 2)
        {
            if(strcmp(command[0],"stat") == 0)
            {
                ret = stat_file(command[1]);
                if(ret == -1)
                    printf("ERROR : Incorrect parameters\n");
                continue;
            }
            else if(strcmp(command[0],"fstat") == 0)
            {
                ret = fstat_file(atoi(command[1]));
                if(ret == -1)
                    printf("ERROR : Incorrect parameters\n");
                if(ret == -2)
                    printf("ERROR : There is no such file\n");
                continue;
            }
            else if(strcmp(command[0],"close") == 0)
            {
                ret = W_CloseFileByName(command[1]);
                if(ret == -1)
                    printf("ERROR : There is no such file\n");
                continue;
            }
            else if(strcmp(command[0],"rm") == 0)
            {
                ret = W_rm_File(command[1]);
                if(ret == -1)
                    printf("ERROR : There is no such file\n");
                continue;
            }
            else if(strcmp(command[0],"man") == 0)
            {
                man(command[1]);
            }
            else if(strcmp(command[0],"write") == 0)
            {
                fd = GetFDFromName(command[1]);
                if(fd == -1)
                {
                    printf("ERROR : Incorrect parameter\n");
                    continue;
                }
                printf("Enter the data : \n");
                scanf("%[^'\n']s", arr);

                ret = strlen(arr);
                if(ret == 0)
                {
                    printf("ERROR : Incorrect parameter\n");
                    continue;
                }
                ret = W_WriteFile(fd,arr,ret);
                if(ret == -1)
                    printf("ERROR : Permission denied\n");
                if(ret == -2)
                    printf("ERROR : There is no sufficient memory to write\n");
                if(ret == -3)
                    printf("ERROR : It is not regular file\n");
            }
            else if(strcmp(command[0],"truncate") == 0)
            {
                ret = W_truncate_File(command[1]);
                if(ret == -1)
                    printf("ERROR : Incorrect parameter\n");
            }
            else if(strcmp(command[0],"cd") == 0)
            {
                if (dir_change(command[1]) != 0)
                    printf("ERROR : Directory not found\n");
            }
            else if(strcmp(command[0],"mkdir") == 0)
            {
                ret = dir_create(command[1]);
                if(ret < 0)
                    printf("ERROR : Could not create directory (%d)\n", ret);
            }
            else if(strcmp(command[0],"rmdir") == 0)
            {
                ret = dir_remove(command[1]);
                if(ret < 0)
                    printf("ERROR : Could not remove directory (%d)\n", ret);
            }
            else
            {
                printf("\nERROR : Command not found\n");
                    continue;
            }
        }
        else if(count == 3)
        {
            if(strcmp(command[0],"create") == 0)
            {
                ret = W_CreateFile(command[1],atoi(command[2]));
                if(ret >= 0)
                    printf("File is successfully created with file descriptor : %d\n", ret);
                if(ret == -1)
                    printf("ERROR : Incorrect parameters\n");
                if(ret == -2)
                    printf("ERROR : There is no inodes\n");
                if(ret == -3)
                    printf("ERROR : File already exists\n");
                if(ret == -4)
                    printf("ERROR : Memory allocation failure\n");
                continue;
            }
            else if(strcmp(command[0],"open") == 0)
            {
                ret = W_OpenFile(command[1],atoi(command[2]));
                if(ret >= 0)
                    printf("File is successfully opened with file descriptor : %d\n", ret);
                if(ret == -1)
                    printf("ERROR : Incorrect parameters\n");
                if(ret == -2)
                    printf("ERROR : File not present\n");
                if(ret == -3)
                    printf("ERROR : Permission denied\n");
                continue;
            }
            else if(strcmp(command[0],"read") == 0)
            {
                fd = GetFDFromName(command[1]);
                if(fd == -1)
                {
                    printf("ERROR : Incorrect parameters\n");
                    continue;
                }
                ptr = (char *)malloc(sizeof(atoi(command[2])) + 1);
                if(ptr == NULL)
                {
                    printf("ERROR : Memory allocation failure\n");
                    continue;
                }
                ret = W_ReadFile(fd,ptr,atoi(command[2]));
                if(ret == -1)
                    printf("ERROR : File not existing\n");
                if(ret == -2)
                    printf("ERROR : Permission denied\n");
                if(ret == -3)
                    printf("ERROR : Reached at end of file\n");
                if(ret == -4)
                    printf("ERROR : It is not regular file\n");
                if(ret == 0)
                    printf("ERROR : File empty\n");
                if(ret > 0)
                {
                    printf("%.*s", ret, ptr);
                }
                continue;
            }
            else
            {
                printf("\nERROR : Command not found !!!\n");
                continue;
            }
        }
        else if(count == 4)
        {
            if(strcmp(command[0],"lseek") == 0)
            {
                fd = GetFDFromName(command[1]);
                if(fd == -1)
                {
                    printf("ERROR : Incorect parameters\n");
                    continue;
                }
                ret = W_LseekFile(fd,atoi(command[2]),atoi(command[3]));
                if(ret == -1)
                {
                    printf("ERROR : Unable to perform lseek\n");
                }
            }
            else 
            {
                printf("\nERROR : Command not found !!!\n");
                continue;
            }
        }
        else 
        {
            printf("\nERROR : Command not found\n");
            continue;
        }
    }

    CVFS_Shutdown();
    return 0;
}
