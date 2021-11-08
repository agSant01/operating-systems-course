// fs.cpp: File System
#include "sfs/fs.h"

// #include <algorithm>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

int *bitmap;
Disk *selfDisk;
Block *superBlock;

bool hasDiskMounted()
{
    return selfDisk != NULL && selfDisk->mounted(selfDisk);
}

bool loadInode(size_t inumber, Inode *inode)
{
    if (inumber >= superBlock->Super.Inodes)
    {
        return false;
    }
    size_t blockNumber = (inumber / INODES_PER_BLOCK) + 1;
    Block block;
    selfDisk->readDisk(selfDisk, blockNumber, block.Data);
    memcpy(inode, &block.Inodes[inumber % INODES_PER_BLOCK], sizeof(Inode));
    return true;
}

// Debug file system -----------------------------------------------------------

void debug(Disk *disk)
{
    Block block;

    // Read Superblock
    printf("    %ld\n", disk->Blocks);
    disk->readDisk(disk, 0, block.Data);

    printf("SuperBlock:\n");

    if (block.Super.MagicNumber == MAGIC_NUMBER)
    {
        printf("    magic number is valid\n");
    }
    else
    {
        printf("    magic number is not valid\n");
    }

    printf("    %u blocks\n", block.Super.Blocks);
    printf("    %u inode blocks\n", block.Super.InodeBlocks);
    printf("    %u inodes\n", block.Super.Inodes);

    // Read Inode blocks
    uint32_t iblock = 1;
    uint32_t totalIBlocks = block.Super.InodeBlocks;
    ushort inodeIdx = 0;
    Inode inode;
    // printf("block : %d | total: %d\n", iblock, totalIBlocks); // debug
    while (iblock <= totalIBlocks)
    {
        disk->readDisk(disk, iblock, block.Data);
        inodeIdx = 0;

        while (inodeIdx < INODES_PER_BLOCK)
        {
            // printf("IBlock: %d\n", inodeIdx); // debug
            inode = block.Inodes[inodeIdx];
            if (inode.Valid == 1)
            {
                printf("Inode %d:\n", iblock * inodeIdx);
                printf("    size: %u bytes\n", inode.Size);

                ushort directBlocks = 0;
                for (ushort idx = 0; idx < POINTERS_PER_INODE; idx++)
                {
                    // debug
                    // printf("direct %d | %d\n", inode.Direct[idx], idx);
                    if (inode.Direct[idx] != 0)
                        directBlocks++;
                }

                printf("    direct blocks: %u\n", directBlocks);
            }
            inodeIdx++;
        }
        iblock++;
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
    uint32_t blockIdx = 1;
    while (blockIdx < disk->Blocks)
    {
        disk->writeDisk(disk, blockIdx, block.Data);
        blockIdx++;
    }

    return true;
}

// Mount file system -----------------------------------------------------------

bool mount(Disk *disk)
{
    if (disk->mounted(disk))
    {
        return false;
    }

    Block block;
    uint32_t inodeBlock = 1;
    ushort inodeIdx;
    Inode inode;

    // Read superblock
    free(superBlock);
    superBlock = calloc(1, sizeof(Block));
    disk->readDisk(disk, 0, superBlock->Data);
    if (superBlock->Super.MagicNumber != MAGIC_NUMBER)
        return false;

    // Set device and mount
    disk->mount(disk);
    selfDisk = disk;

    // Copy metadata
    // what to do???

    // Allocate free block bitmap
    free(bitmap);

    bitmap = calloc(superBlock->Super.Blocks, sizeof(int));

    bitmap[0] = 1; // mark super block as used

    while (inodeBlock <= superBlock->Super.InodeBlocks)
    {
        bitmap[inodeBlock] = 1; // mark inode as used
        inodeIdx = 0;
        disk->readDisk(disk, inodeBlock++, block.Data);
        while (inodeIdx < INODES_PER_BLOCK)
        {
            inode = block.Inodes[inodeIdx++];

            if (inode.Valid == 0)
            {
                continue;
            }

            for (ushort direct = 0; direct < POINTERS_PER_INODE; direct++)
            {
                if (inode.Direct[direct] != 0)
                {
                    bitmap[inode.Direct[direct]] = 1; // mark data block as used
                }
            }
        }
    }

    printf("HI\n");
    for (int i = 0; i < superBlock->Super.Blocks; i++)
    {
        printf("%d => %d\n", i, bitmap[i]);
    }

    return true;
}

// Create inode ----------------------------------------------------------------

size_t create()
{
    // if ()
    // {
    // }

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
    if (!hasDiskMounted())
    {
        return 0;
    }

    // Load inode information
    Inode inode;
    if (!loadInode(inumber, &inode))
    {
        return 0;
    }
    // printf("Loaded inode...\n"); // debug

    if (inode.Valid == 0)
    {
        return 0;
    }
    // printf("Inode is valid...\n"); // debug

    // Adjust length
    char *buffer;
    buffer = (char *)calloc(length, sizeof(char));

    uint32_t startBlock = offset / BLOCK_SIZE;
    Block block;

    // printf("StartBlock %d\n", startBlock); // debug
    while (startBlock < POINTERS_PER_INODE)
    {
        if (inode.Direct[startBlock] == 0)
            break;

        bzero(&block, BLOCK_SIZE);
        selfDisk->readDisk(selfDisk, inode.Direct[startBlock], block.Data);

        if (strlen(buffer) + strlen(block.Data) >= length)
            break;

        strncat(buffer, block.Data + offset, length - strlen(buffer));

        offset = 0;
        startBlock++;
    }

    // Read block and copy to data
    strcpy(data, buffer);
    free(buffer);

    return strlen(data);
}

// Write to inode --------------------------------------------------------------

size_t writeInode(size_t inumber, char *data, size_t length, size_t offset)
{
    // Load inode

    // Write block and copy to data
    return 0;
}
