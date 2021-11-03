// fs.cpp: File System
#include <math.h>

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
    // printf("    %u\n", block.Super.Blocks);
    disk->readDisk(disk, 0, block.Data);

    printf("SuperBlock:\n");
    printf("    %u blocks\n", block.Super.Blocks);
    printf("    %u inode blocks\n", block.Super.InodeBlocks);
    printf("    %u inodes\n", block.Super.Inodes);

    // Read Inode blocks
    uint32_t inodeBlockIdx = 1;
    ushort inodeIdx = 0;
    Inode inode;
    while (inodeBlockIdx <= block.Super.InodeBlocks)
    {
        disk->readDisk(disk, inodeBlockIdx, block.Data);
        inodeIdx = 0;

        while (inodeIdx < 128)
        {
            inode = block.Inodes[inodeIdx];
            if (inode.Valid == 1)
            {
                printf("Inode %d:\n", inodeBlockIdx * inodeIdx);
                printf("    size: %u bytes\n", inode.Size);

                ushort directBlocks = 0;
                for (ushort idx = 0; idx < POINTERS_PER_INODE; idx++)
                    if (inode.Direct[idx] != 0)
                        directBlocks++;

                printf("    direct blocks: %u\n", directBlocks);
            }
            inodeIdx++;
        }
        inodeBlockIdx++;
    }
}

// Format file system ----------------------------------------------------------

bool format(Disk *disk)
{
    Block block;

    uint32_t inodeBlocks = (uint32_t)ceil(0.10 * disk->Blocks);

    block.Super.MagicNumber = MAGIC_NUMBER;
    block.Super.Blocks = disk->Blocks;
    block.Super.InodeBlocks = inodeBlocks;
    block.Super.Inodes = INODES_PER_BLOCK * inodeBlocks;

    // Write superblock
    disk->writeDisk(disk, 0, block.Data);

    // clear block data
    bzero(block.Data, BLOCK_SIZE);

    // Clear all other blocks
    uint32_t dataInode = inodeBlocks;
    while (dataInode < disk->Blocks)
    {
        disk->writeDisk(disk, dataInode, block.Data);
        printf("data block %d\n", dataInode);
        dataInode++;
    }

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
