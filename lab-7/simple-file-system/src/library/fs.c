// fs.cpp: File System

#include "sfs/fs.h"

// #include <algorithm>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Debug file system -----------------------------------------------------------

void debug(Disk *disk)
{
    Block block;

    // Read Superblock

    disk->readDisk(disk, 0, block.Data);

    printf("SuperBlock:\n");
    printf("    %u blocks\n", block.Super.Blocks);
    printf("    %u inode blocks\n", block.Super.InodeBlocks);
    printf("    %u inodes\n", block.Super.Inodes);

    // Read Inode blocks
}

// Format file system ----------------------------------------------------------

bool format(Disk *disk)
{

    // Write superblock

    // Clear all other blocks

    return true;
}

// Mount file system -----------------------------------------------------------

bool mount(Disk *disk)
{
    // Read superblock

    // Set device and mount

    // Copy metadata

    // Allocate free block bitmap

    return true;
}

// Create inode ----------------------------------------------------------------

size_t create()
{
    // Locate free inode in inode table

    // Record inode if found
    return 0;
}

// Remove inode ----------------------------------------------------------------

bool removeInode(size_t inumber)
{
    // Load inode information

    // Free direct blocks

    // Free indirect blocks

    // Clear inode in inode table
    return true;
}

// Inode stat ------------------------------------------------------------------

size_t stat(size_t inumber)
{
    // Load inode information
    return 0;
}

// Read from inode -------------------------------------------------------------

size_t readInode(size_t inumber, char *data, size_t length, size_t offset)
{
    // Load inode information

    // Adjust length

    // Read block and copy to data
    return 0;
}

// Write to inode --------------------------------------------------------------

size_t writeInode(size_t inumber, char *data, size_t length, size_t offset)
{
    // Load inode

    // Write block and copy to data
    return 0;
}
