#include <sys/cdefs.h>
#include <fcntl.h>
#include <lib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <strings.h>
#include "mfs/const.h"
#include "mfs/inode.h"
#include "mfs/type.h"
#include "mfs/mfsdir.h"
#include "mfs/super.h"

//static struct super_block sb;


int read_superblock(int dfd, struct super_block *sb){
	if(lseek(dfd, SUPER_BLOCK_BYTES, SEEK_SET) != SUPER_BLOCK_BYTES){
		perror("lseek super block");
		return -1;
	}
	if(read(dfd, sb, sizeof(*sb)) != sizeof(*sb)){
		perror("error read superblock");
		return -1;
	}
	if(sb->s_magic != SUPER_V3){
		perror("bad magic superblock");
		return -1;
	} 
	return 0;
}

int get_device_file(dev_t dev_id){

	char path[] = "/dev/";
    struct dirent *direntp = NULL;
    DIR *dirp = NULL;
    size_t path_len;

    path_len = strlen(path);    

    dirp = opendir(path);
    if (dirp == NULL){
    	perror("opendir /dev");
        return -1;
    }

    while ((direntp = readdir(dirp)) != NULL)
    {
        /* For every directory entry... */
        struct stat fstat;
        char full_name[_POSIX_PATH_MAX + 1];

        /* Ignore special directories. */
        if ((strcmp(direntp->d_name, ".") == 0) ||
            (strcmp(direntp->d_name, "..") == 0))
            continue;

        /* Calculate full name, check we are in file length limts */
        if ((path_len + strlen(direntp->d_name) + 1) > _POSIX_PATH_MAX)
            continue;

        strcpy(full_name, path);
        strcat(full_name, direntp->d_name);

        /* print */
        if (stat(full_name, &fstat) < 0)
            continue;

		if (S_ISBLK(fstat.st_mode) && fstat.st_rdev == dev_id){
			closedir(dirp);
			return open(full_name, O_RDWR);
        }

    }

    /* Finalize resources. */
    closedir(dirp);
    fprintf(stderr, "Couldnt find device\n");
    return -1;
}

int get_blocks(int tab[V2_NR_TZONES], dev_t dev_id, ino_t inode_id){
	int dfd = get_device_file(dev_id);
	if(dfd == -1)
		return -1;
	struct super_block sb;
	if(read_superblock(dfd, &sb) == -1)
		return -1;
	int offset = (START_BLOCK + sb.s_imap_blocks + sb.s_zmap_blocks) * sb.s_block_size + (inode_id-1)*V2_INODE_SIZE;
	if(lseek(dfd, offset, SEEK_SET) != offset){
		perror("lseek inode");
		return -1;
	}
	struct inode i;
	if(read(dfd, &i, sizeof(i)) != sizeof(i)){
		perror("error read inode");
		return -1;
	}
	for(int d = 0; d < V2_NR_TZONES; d++){
		tab[d] = i.i_zone[d];
	}
	close(dfd);
	return 0;	
}


int view_directory(const char *path, int recursive)
{
    struct dirent *direntp = NULL;
    DIR *dirp = NULL;
    size_t path_len;
    int tab[V2_NR_TZONES];

    if (!path)
        return -1;
    path_len = strlen(path);    

    if (!path || !path_len || (path_len > _POSIX_PATH_MAX))
        return -1;

    dirp = opendir(path);
    if (dirp == NULL)
        return -1;

    while ((direntp = readdir(dirp)) != NULL)
    {
        /* For every directory entry... */
        struct stat fstat;
        char full_name[_POSIX_PATH_MAX + 1];

        /* Calculate full name, check we are in file length limts */
        if ((path_len + strlen(direntp->d_name) + 1) > _POSIX_PATH_MAX)
            continue;

        strcpy(full_name, path);
        if (full_name[path_len - 1] != '/')
            strcat(full_name, "/");
        strcat(full_name, direntp->d_name);

        /* Ignore special directories. */
        if ((strcmp(direntp->d_name, ".") == 0) ||
            (strcmp(direntp->d_name, "..") == 0))
            continue;

        /* print */
        if (stat(full_name, &fstat) < 0)
            continue;

        mode_t m = fstat.st_mode;

        if (S_ISDIR(m)){
            printf("%s -- (%llu:%llu) -- dir -- ", full_name, fstat.st_dev, fstat.st_ino);
        }
        else if (S_ISREG(m)) {
            printf("%s -- (%llu:%llu) -- file -- ", full_name, fstat.st_dev, fstat.st_ino);           
        }
        else if (S_ISCHR(m)) {
            printf("%s -- (%llu:%llu) -- char -- ", full_name, fstat.st_dev, fstat.st_ino);
        }
        else if (S_ISBLK(m)) {
            printf("%s -- (%llu:%llu) -- block -- ", full_name, fstat.st_dev, fstat.st_ino);
        }
        else if (S_ISFIFO(m)) {
            printf("%s -- (%llu:%llu) -- pipe -- ", full_name, fstat.st_dev, fstat.st_ino);
        }
        else if (S_ISLNK(m)) {
            printf("%s -- (%llu:%llu) -- link -- ", full_name, fstat.st_dev, fstat.st_ino);
        }
        else if (S_ISSOCK(m)) {
            printf("%s -- (%llu:%llu) -- socket -- ", full_name, fstat.st_dev, fstat.st_ino);
        }

        if(get_blocks(tab, fstat.st_dev, fstat.st_ino) == -1){
        	printf("get block not working\n");
        	continue;
        }
        for(int d = 0; d < V2_NR_TZONES; d++){
        	printf("%d ", tab[d]);
        }
        printf("\n");
        if (S_ISDIR(m) && recursive)
            view_directory(full_name, 1);
    }

    /* Finalize resources. */
    closedir(dirp);
    return 0;
}

int directoryWalker(int r){
	char path[50];
	bzero(path,50);
	printf("Enter the path you want to open: \n");
	scanf(" %[^\n]%*c", path);
	view_directory(path, r);
	return 0;
}

int main(){
	int input=0;

    printf("\nFS TOOLS\n");
	do{
		printf("\nPlease enter \n \
			1 directory walker\n \
			0 exit\n");
		printf(">");
		scanf("%d",&input);
		switch(input){

			case 1:{
			printf("\nPlease enter \n \
			1 recursion\n \
			0 no recursion\n");
			scanf("%d",&input);
			directoryWalker(input);
			}
			break;
			case 0:
			exit(0);
			break;
			default:
			break;
		}
	}while(input !=0);
}