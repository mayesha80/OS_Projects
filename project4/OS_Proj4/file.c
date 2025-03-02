#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "API.h"
#include "disk.h"

int file_cat(char *name)
{
		int inodeNum, i, size;
		int block, numBlock;
		char str_buffer[512];
		char * str;

		//get inode
		inodeNum = search_cur_dir(name);
		size = inode[inodeNum].size;

		//check if valid input
		if(inodeNum < 0)
		{
				printf("cat error: file not found\n");
				return -1;
		}
		if(inode[inodeNum].type == directory)
		{
				printf("cat error: cannot read directory\n");
				return -1;
		}

		//allocate str
		str = (char *) malloc( sizeof(char) * (size+1) );
		str[ size ] = '\0';

		for( i = 0; i < inode[inodeNum].blockCount; i++ ){
				int block;
				block = inode[inodeNum].directBlock[i];

				read_disk_block( block, str_buffer );

				if( size >= BLOCK_SIZE )
				{
						memcpy( str+i*BLOCK_SIZE, str_buffer, BLOCK_SIZE );
						size -= BLOCK_SIZE;
				}
				else
				{
						memcpy( str+i*BLOCK_SIZE, str_buffer, size );
				}
		}
		printf("%s\n", str);

		//update lastAccess
		gettimeofday( &(inode[inodeNum].lastAccess), NULL );

		free(str);

		//return success
		return 0;
}

int search_cur_dir_index(int inodeNum)
{
		// return inode. If not exist, return -1
		int i;

		for(i = 0; i < curDir.numEntry; i++)
		{
				if(inodeNum == curDir.dentry[i].inode) return i;
		}
		return -1;
}

void cleanInode(int inodeNum, int shift) {
	if(inode[inodeNum].type == file) {
		--inode[inodeNum].linkCount;
		if(inode[inodeNum].linkCount == 0) {
			int i;
			for(i = 0; i < inode[inodeNum].blockCount; ++i) {
				int blk = inode[inodeNum].directBlock[i];
				free_block(blk);
			}
			free_inode(inodeNum);
		}
	} else {
		Dentry removeDir;
		read_disk_block(inode[inodeNum].directBlock[0], (char*)&removeDir);
		int i;
		//skipping 1st 2 for own and parent reference
		for(i = 2; i < removeDir.numEntry; ++i) {
			cleanInode(removeDir.dentry[i].inode, 0);
		}
		free_block(inode[inodeNum].directBlock[0]);
		free_inode(inodeNum);
	}
	if(shift) {
		int idx = search_cur_dir_index(inodeNum);
		int i;
		// move director entry to left for all right from this position
		for(i = idx + 1; i < curDir.numEntry; ++i) {
			curDir.dentry[i - 1] = curDir.dentry[i]; 
		}
		--curDir.numEntry;
	}
}

int file_remove(char *name)
{
	int block, numBlock;
	int inodeNum = search_cur_dir(name); 
	if(inodeNum < 0) {
		printf("remove error: file not found\n");
		return -1;
	}
	
	if(inode[inodeNum].type == directory) {
		printf("remove error: cannot remove directory\n");
		return -1;
	}
	cleanInode(inodeNum, 1);
	//update last access of current directory
	gettimeofday(&(inode[curDir.dentry[0].inode].lastAccess), NULL);

	printf("File removed: %s\n", name);
	return 0;
}

int hard_link(char *src, char *dest)
{
	int srcInode = search_cur_dir(src); 
	if(srcInode < 0) {
		printf("link error: file not found\n");
		return -1;
	}
	
	if(inode[srcInode].type == directory) {
		printf("link error: cannot link directory\n");
		return -1;
	}
	int inodeNum = search_cur_dir(dest);
	if(inodeNum >= 0) {
		printf("File link failed:  %s exist.\n", dest);
		return -1;
	}
	if(curDir.numEntry >= MAX_DIR_ENTRY) {
		printf("File link failed: directory is full!\n");
		return -1;
	}
	// add a new file into the current directory entry
	strncpy(curDir.dentry[curDir.numEntry].name, dest, strlen(dest));
	curDir.dentry[curDir.numEntry].name[strlen(dest)] = '\0';
	curDir.dentry[curDir.numEntry].inode = srcInode;
	printf("curdir %s, name %s\n", curDir.dentry[curDir.numEntry].name, dest);
	curDir.numEntry++;
	++inode[srcInode].linkCount;
	//update last access of current directory
	gettimeofday(&(inode[curDir.dentry[0].inode].lastAccess), NULL);
	
	return 0;
}

int file_copy(char *src, char *dest)
{
	int srcInode = search_cur_dir(src);
	if(srcInode < 0) {
		printf("copy error: source file not found\n");
		return -1;
	}

	if(inode[srcInode].type == directory) {
		printf("copy error: cannot copy a directory\n");
		return -1;
	}

	int destInode = search_cur_dir(dest);
	if(destInode >= 0) {
		printf("copy error: destination file already exists\n");
		return -1;
	}

	if(curDir.numEntry >= MAX_DIR_ENTRY) {
		printf("copy error: directory is full!\n");
		return -1;
	}

	if(superBlock.freeInodeCount < 1) {
		printf("copy error: inode is full!\n");
		return -1;
	}

	int newInode = get_inode();
	if(newInode < 0) {
		printf("copy error: could not allocate inode\n");
		return -1;
	}

	inode[newInode] = inode[srcInode]; // Copy metadata
	inode[newInode].linkCount = 1;

	// Allocate blocks for the new file
	int i;
	for(i = 0; i < inode[srcInode].blockCount; ++i) {
		int newBlock = get_block();
		if(newBlock == -1) {
			printf("copy error: get_block failed\n");
			return -1;
		}
		char buffer[BLOCK_SIZE];
		read_disk_block(inode[srcInode].directBlock[i], buffer);
		write_disk_block(newBlock, buffer);
		inode[newInode].directBlock[i] = newBlock;
	}

	// Add new file to directory entry
	strncpy(curDir.dentry[curDir.numEntry].name, dest, strlen(dest));
	curDir.dentry[curDir.numEntry].name[strlen(dest)] = '\0';
	curDir.dentry[curDir.numEntry].inode = newInode;
	curDir.numEntry++;
	
	// Update last access time
	gettimeofday(&(inode[newInode].created), NULL);
	gettimeofday(&(inode[newInode].lastAccess), NULL);
	gettimeofday(&(inode[curDir.dentry[0].inode].lastAccess), NULL);

	printf("File copied from %s to %s\n", src, dest);
	return 0;
}


/* =============================================================*/

int file_create(char *name, int size)
{
		int i;
		int block, inodeNum, numBlock;

		if(size <= 0 || size > 73216){
				printf("File create failed: file size error\n");
				return -1;
		}

		inodeNum = search_cur_dir(name); 
		if(inodeNum >= 0) {
				printf("File create failed:  %s exist.\n", name);
				return -1;
		}

		if(curDir.numEntry + 1 > MAX_DIR_ENTRY) {
				printf("File create failed: directory is full!\n");
				return -1;
		}

		if(superBlock.freeInodeCount < 1) {
				printf("File create failed: inode is full!\n");
				return -1;
		}

		numBlock = size / BLOCK_SIZE;
		if(size % BLOCK_SIZE > 0) numBlock++;

		if(size > 7680) {
				if(numBlock+1 > superBlock.freeBlockCount)
				{
						printf("File create failed: data block is full!\n");
						return -1;
				}
		} else {
				if(numBlock > superBlock.freeBlockCount) {
						printf("File create failed: data block is full!\n");
						return -1;
				}
		}

		char *tmp = (char*) malloc(sizeof(int) * size+1);

		rand_string(tmp, size);
		printf("File contents:\n%s\n", tmp);

		// get inode and fill it
		inodeNum = get_inode();
		if(inodeNum < 0) {
				printf("File_create error: not enough inode.\n");
				return -1;
		}
		
		Inode newInode;

		newInode.type = file;
		newInode.size = size;
		newInode.blockCount = numBlock;
		newInode.linkCount = 1;
		inode[inodeNum].owner = 1;  // pre-defined
		inode[inodeNum].group = 2;  // pre-defined

		// add a new file into the current directory entry
		strncpy(curDir.dentry[curDir.numEntry].name, name, strlen(name));
		curDir.dentry[curDir.numEntry].name[strlen(name)] = '\0';
		curDir.dentry[curDir.numEntry].inode = inodeNum;
		curDir.numEntry++;

		// get data blocks
		for(i = 0; i < 15; i++)
		{
				if (i >= numBlock) break;
				block = get_block();
				if(block == -1) {
						printf("File_create error: get_block failed\n");
						return -1;
				}
				//set direct block
				newInode.directBlock[i] = block;

				write_disk_block(block, tmp+(i*BLOCK_SIZE));
		}

		if(size > 7680) {
				// get an indirect block
				block = get_block();
				if(block == -1) {
						printf("File_create error: get_block failed\n");
						return -1;
				}

				newInode.indirectBlock = block;
				int indirectBlockMap[128];

				for(i = 15; i < numBlock; i++)
				{
						block = get_block();
						if(block == -1) {
								printf("File_create error: get_block failed\n");
								return -1;
						}
						//set direct block
						indirectBlockMap[i-15] = block;
						write_disk_block(block, tmp+(i*BLOCK_SIZE));
				}
				write_disk_block(newInode.indirectBlock, (char*)indirectBlockMap);
		}

		write_inode(inodeNum, newInode);
		printf("File created. name: %s, inode: %d, size: %d\n", name, inodeNum, size);

		free(tmp);
		return 0;
}

int file_stat(char *name)
{
		char timebuf[28];
		Inode targetInode;
		int inodeNum;
		
		inodeNum = search_cur_dir(name);
		if(inodeNum < 0) {
				printf("file cat error: file is not exist.\n");
				return -1;
		}
		
		targetInode = read_inode(inodeNum);
		printf("Inode = %d\n", inodeNum);
		if(targetInode.type == file) printf("type = file\n");
		else printf("type = directory\n");
		printf("size = %d\n", targetInode.size);
		printf("linkCount = %d\n", targetInode.linkCount);
		printf("num of block = %d\n", targetInode.blockCount);
}


