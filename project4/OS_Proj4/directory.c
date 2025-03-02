#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "API.h"

int dir_make(char* name)
{
	int inodeNum = search_cur_dir(name); 
	if(inodeNum >= 0) {
		printf("dir create error: %s exist.\n", name);
		return -1;
	}

	if(curDir.numEntry >= MAX_DIR_ENTRY) {
		printf("dir create failed: directory is full!\n");
		return -1;
	}

	if(superBlock.freeBlockCount < 1) {
		printf("dir create failed: data block is full!\n");
		return -1;
	}

	if(superBlock.freeInodeCount < 1) {
		printf("dir create failed: inode is full!\n");
		return -1;
	}
	//Init dir
	int dirInode = get_inode();
	int newDirBlock = get_block();
	inode[dirInode].type = directory;
	inode[dirInode].owner = 0;
	inode[dirInode].group = 0;
	gettimeofday(&(inode[dirInode].created), NULL);
	gettimeofday(&(inode[dirInode].lastAccess), NULL);
	inode[dirInode].size = 1;
	inode[dirInode].blockCount = 1;
	inode[dirInode].directBlock[0] = newDirBlock;
	strncpy(curDir.dentry[curDir.numEntry].name, name, strlen(name));
	curDir.dentry[curDir.numEntry].name[strlen(name)] = '\0';
	curDir.dentry[curDir.numEntry].inode = dirInode;
	++curDir.numEntry;

	Dentry newDir;
	strncpy(newDir.dentry[0].name, ".", 1);
	newDir.dentry[0].name[1] = '\0';
	newDir.dentry[0].inode = dirInode;
	newDir.numEntry = 1;

	strncpy(newDir.dentry[1].name, "..", 2);
	newDir.dentry[1].name[2] = '\0';
	newDir.dentry[1].inode = curDir.dentry[0].inode;
	++newDir.numEntry;

	write_disk_block(newDirBlock, (char*)&newDir);
	
	gettimeofday(&(inode[curDir.dentry[0].inode].lastAccess), NULL);
	printf("Dir created: %s\n", name);

	return 0;
}

int dir_remove(char *name)
{
	int inodeNum = search_cur_dir(name); 

	if(inodeNum < 0) {
		printf("dir remove error: %s not found.\n", name);
		return -1;
	}

	if(inode[inodeNum].type == file) {
		printf("dir remove error: %s is not a directory.\n", name);
		return -1;
	}

	if(command(name, ".") || command(name, "..")) {
		printf("dir remove error: current directory or parent directory can not be deleted.\n", name);
		return -1;
	}

	cleanInode(inodeNum, 1);
	gettimeofday(&(inode[curDir.dentry[0].inode].lastAccess), NULL);

	return 0;
}

int dir_change(char* name)
{
	int inodeNum = search_cur_dir(name); 

	if(inodeNum < 0) {
		printf("dir change error: %s not found.\n", name);
		return -1;
	}
	if(inode[inodeNum].type == file) {
		printf("dir change error: %s is not a directory.\n", name);
		return -1;
	}

	Dentry nextDir;
	read_disk_block(inode[inodeNum].directBlock[0], (char*)&nextDir);
	write_disk_block(curDirBlock, (char*)&curDir);
	curDir = nextDir;
	curDirBlock = inode[inodeNum].directBlock[0];
	gettimeofday(&(inode[inodeNum].lastAccess), NULL);
	printf("Current dir %s\n", name);
	return 0;
}


/* ===================================================== */

int ls()
{
		int i;
		int inodeNum;
		Inode targetInode;
		for(i = 0; i < curDir.numEntry; i++)
		{
				inodeNum = curDir.dentry[i].inode;
				targetInode = read_inode(inodeNum);
				if(targetInode.type == file) printf("type: file, ");
				else printf("type: dir, ");
				printf("name \"%s\", inode %d, size %d byte\n", curDir.dentry[i].name, curDir.dentry[i].inode, targetInode.size);
		}

		return 0;
}


