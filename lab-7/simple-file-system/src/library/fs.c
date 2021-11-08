// fs.cpp: File System
#include "sfs/fs.h"

// #include <algorithm>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

ushort *freeblkmap;
ushort *inodetable;
Disk *selfDisk;
Block *superBlock;

bool hasDiskMounted()
{
    return selfDisk != NULL && selfDisk->mounted(selfDisk);
}

bool initFreeBlocks()
{
    // Allocate free block freeblkmap
    Block block;
    ushort inodeIdx;
    uint32_t inodeBlock = 1;
    Inode inode;

    free(freeblkmap);
    freeblkmap = calloc(superBlock->Super.Blocks, sizeof(ushort));
    freeblkmap[0] = 1; // mark super block as used

    size_t inodesperblock = superBlock->Super.Inodes / superBlock->Super.InodeBlocks;

    while (inodeBlock <= superBlock->Super.InodeBlocks)
    {
        freeblkmap[inodeBlock] = 1; // mark inode as used
        inodeIdx = 0;
        selfDisk->readDisk(selfDisk, inodeBlock++, block.Data);
        while (inodeIdx < inodesperblock)
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
                    freeblkmap[inode.Direct[direct]] = 1; // mark data block as used
                }
            }
        }
    }

    return true;
}

ssize_t allocFreeBlock()
{
    for (int i = superBlock->Super.InodeBlocks; i < superBlock->Super.Blocks; i++)
    {
        if (freeblkmap[i] == 0)
        {
            freeblkmap[i] = 1;
            return i;
        }
    }

    return -1;
}

bool initInodeTable()
{
    free(inodetable);
    inodetable = calloc(superBlock->Super.Inodes, sizeof(ushort));
    size_t inodeidx = 1;
    uint32_t inodeperblk = superBlock->Super.Inodes / superBlock->Super.InodeBlocks;
    Block block;
    for (size_t inodeblk = 1; inodeblk <= superBlock->Super.InodeBlocks; inodeblk++)
    {
        selfDisk->readDisk(selfDisk, inodeblk, block.Data);
        for (size_t inode = 0; inode < inodeperblk; inode++)
        {
            if (block.Inodes[inode].Valid == 1)
            {
                inodetable[(inodeblk - 1) * inodeperblk + inode] = 1;
            }
        }
    }

    for (int i = 0; i < superBlock->Super.Inodes; i++)
        printf("%d %d\n", i, inodetable[i]);

    return true;
}

ssize_t allocFreeInode()
{
    size_t tinodes = superBlock->Super.Blocks * superBlock->Super.InodeBlocks;
    for (int i = 0; i < tinodes; i++)
    {
        if (inodetable[i] == 0)
        {
            inodetable[i] = 1;
            return i;
        }
    }

    return -1;
}

bool loadInode(size_t inumber, Inode *inode)
{
    if (!hasDiskMounted())
    {
        return false;
    }

    if (inumber < 0 || inumber >= superBlock->Super.Inodes)
    {
        return false;
    }

    size_t inodeperblk = superBlock->Super.Inodes / superBlock->Super.InodeBlocks;
    size_t blockNumber = (inumber / inodeperblk) + 1;
    Block block;
    selfDisk->readDisk(selfDisk, blockNumber, block.Data);
    memcpy(inode, &block.Inodes[inumber % inodeperblk], sizeof(Inode));

    return true;
}

bool saveInode(size_t inumber, Inode *inode)
{
    if (!hasDiskMounted())
    {
        return false;
    }

    if (inumber < 0 || inumber >= superBlock->Super.Inodes)
    {
        return false;
    }

    size_t blockNumber = (inumber / INODES_PER_BLOCK) + 1;

    Block block;

    selfDisk->readDisk(selfDisk, blockNumber, block.Data);

    memcpy(&block.Inodes[inumber], inode, sizeof(Inode));

    selfDisk->writeDisk(selfDisk, blockNumber, block.Data);

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

    // Read superblock
    free(superBlock);
    superBlock = calloc(1, sizeof(Block));
    disk->readDisk(disk, 0, superBlock->Data);
    if (superBlock->Super.MagicNumber != MAGIC_NUMBER)
        return false;
    // printf("Valid magic number...\n"); // debug

    // Set device
    selfDisk = disk;
    // Copy metadata
    // what to do???

    // Allocate free block freeblkmap
    if (!initFreeBlocks())
        return false;
    // printf("Init free block table...\n"); // debug

    // Allocate inode table
    if (!initInodeTable())
        return false;
    printf("Init inode table...\n"); // debug

    // Mount
    disk->mount(disk);

    for (int i = 0; i < superBlock->Super.Blocks; i++)
    {
        printf("%d => %d\n", i, freeblkmap[i]);
    }

    return true;
}

// Create inode ----------------------------------------------------------------

ssize_t create()
{
    if (!hasDiskMounted())
    {
        return -1;
    }

    // Locate free inode in inode table
    ssize_t inodeidx = allocFreeInode();
    if (inodeidx < 0)
        return -1;

    size_t inodeperblk = superBlock->Super.Inodes / superBlock->Super.InodeBlocks;
    size_t bnumber = inodeidx / inodeperblk + 1;

    // Record inode if found
    Block block;
    selfDisk->readDisk(selfDisk, bnumber, block.Data);

    size_t adjustedinumber = inodeidx % inodeperblk;

    bzero(&block.Inodes[adjustedinumber], sizeof(Inode));
    block.Inodes[adjustedinumber].Valid = 1;
    printf("bnum %d\n", bnumber);
    selfDisk->writeDisk(selfDisk, bnumber, block.Data);
    printf("inode valid %d\n", block.Inodes[adjustedinumber].Valid);

    return inodeidx;
}

// Remove inode ----------------------------------------------------------------

bool removeInode(size_t inumber)
{
    // Load inode information
    Inode inode;
    if (!loadInode(inumber, &inode))
    {
        return false;
    }

    if (inode.Valid == 0)
    {
        return false;
    }

    // Free direct blocks
    for (int direct = 0; direct < POINTERS_PER_INODE; direct++)
    {
        freeblkmap[inode.Direct[direct]] = 0;

        inode.Direct[direct] = 0;
    }

    // Free indirect blocks
    if (inode.Indirect != 0)
    {
        freeblkmap[inode.Indirect] = 0;
        Block indirectBlk;
        selfDisk->readDisk(selfDisk, inode.Indirect, indirectBlk.Data);
        for (size_t pointer = 0; pointer < POINTERS_PER_BLOCK; pointer++)
        {
            freeblkmap[indirectBlk.Pointers[pointer]] = 0;
            indirectBlk.Pointers[pointer] = 0;
        }
        selfDisk->writeDisk(selfDisk, inode.Indirect, indirectBlk.Data);
        inode.Indirect = 0;
    }

    // Clear inode in inode table
    inodetable[inumber] = 0;
    inode.Size = 0;
    inode.Valid = 0;

    saveInode(inumber, &inode);

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

        selfDisk->readDisk(selfDisk, inode.Direct[startBlock], block.Data);

        if (strlen(buffer) + strlen(block.Data) >= length)
            break;

        strncat(buffer, block.Data + offset, length - strlen(buffer));

        bzero(&block, BLOCK_SIZE);
        offset = 0;
        startBlock++;
    }

    // Read block and copy to data
    strcpy(data, buffer);
    free(buffer);

    return strlen(data);
}

// Write to inode --------------------------------------------------------------

ssize_t writeInode(size_t inumber, char *data, size_t length, size_t offset)
{
    // Load inode
    Inode inode;
    if (!loadInode(inumber, &inode))
    {
        return -1;
    }

    if (inode.Valid == 0)
    {
        return -1;
    }

    // Write block and copy to data
    Block block;
    int direct = 0;
    size_t written = 0,
           freeblk;

    while (written < length && direct < POINTERS_PER_INODE)
    {
        strncpy(block.Data, data + written, BLOCK_SIZE);
        freeblk = allocFreeBlock();

        if (freeblk <= 0)
            return -1;

        selfDisk->writeDisk(selfDisk, freeblk, block.Data);
        inode.Direct[direct] = freeblk;
        written += strlen(block.Data);
        direct++;
    }

    if (written < length)
    {
        // use indirect
        size_t indblk = allocFreeBlock();

        if (indblk <= 0)
            return -1;
        inode.Indirect = indblk;

        Block pointers;

        for (size_t indirect = 0; indirect < POINTERS_PER_BLOCK; indirect++)
        {
            strncpy(block.Data, data + written, BLOCK_SIZE);
            freeblk = allocFreeBlock();

            if (freeblk <= 0)
                return -1;

            selfDisk->writeDisk(selfDisk, freeblk, block.Data);
            pointers.Pointers[indirect] = freeblk;

            written += strlen(block.Data);

            if (written >= length)
                break;

            indirect++;
        }

        selfDisk->writeDisk(selfDisk, indblk, pointers.Data);
    }

    inode.Size = written;
    saveInode(inumber, &inode);

    return written;
}
