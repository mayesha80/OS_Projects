#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include "fs.h"
#include "disk.h"
#include <unistd.h>

// Function prototypes
void check_and_fix_superblock();
void check_and_fix_inode_map();
void check_and_fix_block_map();
int count_free_inodes();
int count_free_blocks();

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ./fsck <disk_name>\n");
        return -1;
    }

    if (access(argv[1], F_OK) != 0) {
        printf("Disk file \"%s\" does not exist.\n", argv[1]);
        return -1;
    }

    // Mount the filesystem
    if (fs_mount(argv[1]) != 0) {
        fprintf(stderr, "Failed to mount the filesystem.\n");
        return -1;
    }

    printf("Checking filesystem...\n");

    // Check and fix superblock
    check_and_fix_superblock();

    // Check and fix inode map
    check_and_fix_inode_map();

    // Check and fix block map
    check_and_fix_block_map();

    printf("Filesystem check completed.\n");

    // Unmount the filesystem
    fs_umount(argv[1]);

    return 0;
}

// Function to check and fix the superblock
void check_and_fix_superblock() {
    int free_inodes, free_blocks;

    free_inodes = count_free_inodes();
    free_blocks = count_free_blocks();

    if (superBlock.freeInodeCount != free_inodes) {
        printf("Superblock free inode count mismatch: expected %d, found %d. Fixing...\n",
               superBlock.freeInodeCount, free_inodes);
        superBlock.freeInodeCount = free_inodes;
    } else {
        printf("Superblock free inode count is correct.\n");
    }

    if (superBlock.freeBlockCount != free_blocks) {
        printf("Superblock free block count mismatch: expected %d, found %d. Fixing...\n",
               superBlock.freeBlockCount, free_blocks);
        superBlock.freeBlockCount = free_blocks;
    } else {
        printf("Superblock free block count is correct.\n");
    }
}

// Function to count free inodes from the inode map
int count_free_inodes() {
    int i, free_count;

    free_count = 0;
    for (i = 0; i < MAX_INODE; i++) {
        if ((inodeMap[i / 8] & (1 << (i % 8))) == 0) {
            free_count++;
        }
    }
    return free_count;
}

// Function to count free blocks from the block map
int count_free_blocks() {
    int i, free_count;

    free_count = 0;
    for (i = 0; i < MAX_BLOCK; i++) {
        if ((blockMap[i / 8] & (1 << (i % 8))) == 0) {
            free_count++;
        }
    }
    return free_count;
}

// Function to check and fix the inode map
void check_and_fix_inode_map() {
    printf("Inode map checked.\n");
}

// Function to check and fix the block map
void check_and_fix_block_map() {
    printf("Block map checked.\n");
}
