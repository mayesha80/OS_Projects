#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include "fs.h"
#include "disk.h"

int main(int argc, char **argv)
{
		int i;
		int freeInode, freeData;
		freeInode = freeData = 0;
		bool r;

		if(argc < 2) {
				fprintf(stderr, "usage: ./fs_reader disk_name\n");
				return -1;
		}
		
		fs_mount(argv[1]);
		
		printf("##########################\n");
		printf("Super Block: \n");
		printf("# free inode: %d\n", superBlock.freeInodeCount);
		printf("# free databock: %d\n", superBlock.freeBlockCount);
		printf("##########################\n\n");

		printf("inode map: ");
		for(i = 0; i < MAX_INODE; i++)
		{
				if(i%50 == 0) printf("\n%3d: ", i);
				r = get_bit(inodeMap, i);
				if(!r) freeInode++;
				printf("%d ", r);
		}

		printf("\n# free Inode: %d\n\n", freeInode);

		printf("block map: ");
		for(i = 0; i < MAX_BLOCK; i++)
		{
				if(i%50 == 0) printf("\n%4d: ", i);
				r = get_bit(blockMap, i);
				if(!r) freeData++;
				printf("%d ", r);
		}

		printf("\n# free datablock: %d\n\n", freeData);

		fs_umount(argv[1]);
}

