#include <stdbool.h>
#include "fs.h"

// directory structure and inode for the current directory.
// curDir always points to the current directory. 
// curDirBlock is data block number that contains the current directory.
// It should be properly changed if the user changes the directory by "cd" command.
extern Dentry curDir;
extern int curDirBlock;


// Get one available inode from inode map. 
// It returns the first available inode number, and decreases freeInodeCount in the SuperBlock.
int get_inode();

// It frees a inode and increase freeInodeCount in the SuperBlock.
void free_inode(int inodeNum);

// Get one available data block from block map.
// It returns the first available block number, and decreases freeBlockCount in the SuperBlock.
int get_block();

// It frees a block and increase freeBlockCount in the SuperBlock.
void free_block(int blockNum);


// Return a inode structure.
Inode read_inode(int inodeNum);

// Write a inode structure to inode blocks.
void write_inode(int inodeNum, Inode newInode);


// Read a data block (512bytes) from the disk. 
void read_disk_block(int blockNum, char *buf);

// Write a data block to the disk.
void write_disk_block(int blockNum, char *buf);

// Search "name" from the current directory and return inode number, if exist.
// It returns -1 if "name" is not exist in the current directory.
int search_cur_dir(char *name);

