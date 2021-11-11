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

/**
 *  Cache-like implementation for retrieving most recent accessed INODE
 */
Inode *lastRecentlyUsed;
size_t LRU_INUMBER;

int gw(int a)
{
    return log10((double)a) + 1;
}

void printBitmaps()
{
    if (superBlock == NULL)
    {
        printf("Must mount disk first.\n");
        return;
    }
    int max_;
    printf("\n\n------------------------------- Inodes -------------------------------\n");
    max_ = gw(superBlock->Super.Inodes) + 1;
    for (int i = 0; i < superBlock->Super.Inodes; i++)
    {
        printf("%d", i);
        if (i > 0)
        {
            printf("%-*s", max_ - gw(i), " ");
        }
        else
        {
            printf("%-*s", max_ - 1, " ");
        }
        printf("=> %d\n", inodetable[i]);
    }

    printf("------------------------------- Blocks -------------------------------\n");
    max_ = gw(superBlock->Super.Blocks) + 1;
    for (int i = 0; i < superBlock->Super.Blocks; i++)
    {
        printf("%d", i);
        if (i > 0)
        {
            printf("%-*s", max_ - gw(i), " ");
        }
        else
        {
            printf("%-*s", max_ - 1, " ");
        }
        printf("=> %d\n", freeblkmap[i]);
    }
}

bool hasDiskMounted()
{
    return selfDisk != NULL && selfDisk->mounted(selfDisk);
}

bool initFreeBlocks(Inode *inode)
{
    if (inode->Valid == 0)
    {
        return true;
    }

    for (ushort direct = 0; direct < POINTERS_PER_INODE; direct++)
    {
        if (inode->Direct[direct] != 0)
        {
            freeblkmap[inode->Direct[direct]] = 1; // mark data block as used
        }
    }

    if (inode->Indirect != 0)
    {
        freeblkmap[inode->Indirect] = 1;
        Block indblock;
        selfDisk->readDisk(selfDisk, inode->Indirect, indblock.Data);
        for (size_t pointer = 0; pointer < POINTERS_PER_BLOCK && pointer < selfDisk->Blocks; pointer++)
        {
            if (indblock.Pointers[pointer] != 0)
            {
                freeblkmap[indblock.Pointers[pointer]] = 1;
            }
        }
    }

    return true;
}

/**
 * @brief Validate if input bnumber is valid and return newly allocated block number if input bnumber is "0". If free
 * block is not available, return "-1"
 * 
 * @param bnumber block number to compare, if "0" return new allocated block, if "non-zero" return "bnumber"
 * @return uint32_t 
 */
uint32_t allocFreeBlock(uint32_t bnumber)
{
    if (bnumber != 0)
        return bnumber;

    for (uint32_t i = superBlock->Super.InodeBlocks; i < superBlock->Super.Blocks; i++)
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

    free(freeblkmap);
    freeblkmap = calloc(superBlock->Super.Blocks, sizeof(ushort));
    freeblkmap[0] = 1; // mark super block as used

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
                freeblkmap[inodeblk] = 1;
                initFreeBlocks(&block.Inodes[inode]);
            }
        }
    }

    return true;
}

ssize_t allocFreeInode()
{
    size_t tinodes = superBlock->Super.Inodes;
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

    if (inodetable[inumber] == 0)
    {
        return false;
    }

    /* Cache lookup */
    if (lastRecentlyUsed != NULL && inumber == LRU_INUMBER)
    {
        // printf("Cache hit. Inode:%d...\n", inumber);
        inode = lastRecentlyUsed;
        return true; // Cache hit
    }

    /* Cache miss. Read inode from memory */
    size_t inodeperblk = superBlock->Super.Inodes / superBlock->Super.InodeBlocks;
    size_t blockNumber = (inumber / inodeperblk) + 1;
    Block block;
    selfDisk->readDisk(selfDisk, blockNumber, block.Data);
    memcpy(inode, &block.Inodes[inumber % inodeperblk], sizeof(Inode));

    /* Update inode Cache references */
    lastRecentlyUsed = inode;
    LRU_INUMBER = inumber;

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

    uint32_t inodesperblk = superBlock->Super.Inodes / superBlock->Super.InodeBlocks;
    uint32_t blockNumber = (inumber / inodesperblk) + 1;

    Block block;
    selfDisk->readDisk(selfDisk, blockNumber, block.Data);

    memcpy(&block.Inodes[inumber % inodesperblk], inode, sizeof(Inode));

    selfDisk->writeDisk(selfDisk, blockNumber, block.Data);

    return true;
}

// Debug file system -----------------------------------------------------------

void debug(Disk *disk)
{
    Block block;

    // Read Superblock
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

                printf("    direct blocks:");
                for (ushort idx = 0; idx < POINTERS_PER_INODE; idx++)
                {
                    if (inode.Direct[idx] != 0)
                        printf(" %u", inode.Direct[idx]);
                }
                printf("\n");

                if (inode.Indirect != 0)
                {
                    Block indirect;
                    disk->readDisk(disk, inode.Indirect, indirect.Data);
                    printf("    indirect block: %u\n", inode.Indirect);
                    printf("    indirect data blocks:");
                    for (size_t ind = 0; ind < POINTERS_PER_BLOCK; ind++)
                    {
                        if (indirect.Pointers[ind] != 0)
                            printf(" %u", indirect.Pointers[ind]);
                    }
                    printf("\n");
                }
            }
            inodeIdx++;
        }
        iblock++;
    }
}

// Format file system ----------------------------------------------------------

bool format(Disk *disk)
{
    if (selfDisk != NULL)
    {
        return false;
    }

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
    {
        fprintf(stderr, "Invalid valid magic number: %x...\n", superBlock->Super.MagicNumber); // debug
        return false;
    }
    fprintf(stderr, "Valid magic number: %x...\n", superBlock->Super.MagicNumber); // debug

    if (superBlock->Super.Blocks == 0)
        return false;

    if (superBlock->Super.InodeBlocks != ceil(0.10 * superBlock->Super.Blocks))
        return false;

    uint32_t inodes = superBlock->Super.Inodes;
    if (inodes == 0 || !((inodes & (inodes - 1)) == 0))
        return false;

    // Set device
    selfDisk = disk;
    // Copy metadata
    // what to do???

    // Allocate free block freeblkmap
    // Allocate inode table
    if (!initInodeTable())
        return false;

    // printf("Init inode table...\n"); // debug

    // Mount
    disk->mount(disk);

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

    // Free direct blocks
    for (int direct = 0; direct < POINTERS_PER_INODE; direct++)
    {
        freeblkmap[inode.Direct[direct]] = 0;
        inode.Direct[direct] = 0;
    }
    // printf("freed direct blocks...\n");

    // Free indirect blocks
    if (inode.Indirect != 0)
    {
        freeblkmap[inode.Indirect] = 0;
        Block indirectBlk;
        selfDisk->readDisk(selfDisk, inode.Indirect, indirectBlk.Data);
        for (size_t pointer = 0; pointer < POINTERS_PER_BLOCK && pointer < selfDisk->Blocks; pointer++)
        {
            // printf("pointer: %d...\n", pointer);
            freeblkmap[indirectBlk.Pointers[pointer]] = 0;
            indirectBlk.Pointers[pointer] = 0;
        }
        selfDisk->writeDisk(selfDisk, inode.Indirect, indirectBlk.Data);
        inode.Indirect = 0;
    }
    printf("freed indirect blocks...\n");

    // Clear inode in inode table
    inodetable[inumber] = 0;
    inode.Size = 0;
    inode.Valid = 0;
    printf("cleared inode table...\n");

    saveInode(inumber, &inode);

    return true;
}

// Inode stat ------------------------------------------------------------------

size_t stat(size_t inumber)
{
    // if (selfDisk == NULL)
    // {
    //     fprintf(stderr, "Mount disk first...\n");
    //     return -1;
    // }
    // Load inode information
    Inode inode;
    loadInode(inumber, &inode);
    // if (!loadInode(inumber, &inode))
    // {
    //     fprintf(stderr, "Invalid inode...\n");
    //     return -1;
    // };

    // fprintf(stderr, "Direct:\n");
    // for (int i = 0; i < POINTERS_PER_INODE; i++)
    //     fprintf(stderr, "    [%d] => %ld\n", i, inode.Direct[i]);
    // fprintf(stderr, "Indirect => %ld\n", inode.Indirect);

    // Block block;
    // selfDisk->readDisk(selfDisk, inode.Indirect, block.Data);
    // for (int i = 0; i < POINTERS_PER_BLOCK; i++)
    // {
    //     if (block.Pointers[i] == 0)
    //         break;
    //     fprintf(stderr, "    [%d] => %ld\n", i, block.Pointers[i]);
    // }
    // fprintf(stderr, "Valid = %ld\n", inode.Valid);
    return inode.Size;
}

// Read from inode -------------------------------------------------------------

size_t readInode(size_t inumber, char *data, size_t length, size_t offset)
{
    fprintf(stderr, "readInode(inumber:%ld, length:%ld, offset:%ld)\n", inumber, length, offset);
    // Load inode information
    Inode inode;
    if (!loadInode(inumber, &inode))
    {
        return 0;
    }
    // printf("Loaded inode...\n"); // debug

    // Adjust length
    size_t offsetedDataBlock = offset / BLOCK_SIZE;
    offset %= BLOCK_SIZE;
    Block block;
    int read = 0;

    // printf("StartBlock %d\n", startBlock); // debug
    while (offsetedDataBlock < POINTERS_PER_INODE && read < length)
    {
        if (inode.Direct[offsetedDataBlock] == 0)
            break;
        fprintf(stderr, "Hi %d\n", inode.Direct[offsetedDataBlock]);
        selfDisk->readDisk(selfDisk, inode.Direct[offsetedDataBlock], block.Data);

        strncpy(data + read, block.Data + offset, fmin(BLOCK_SIZE, length - read));
        read += strlen(data + read);

        fprintf(stderr, "Read: %d | Offset:%ld\n", read, offset);
        bzero(block.Data, BLOCK_SIZE);
        offset = 0;
        offsetedDataBlock++;
    }
    // printf("Ended reading directs...\n");

    if (inode.Indirect != 0)
    {
        fprintf(stderr, "Started reading indirects...\n");

        Block pointers;
        selfDisk->readDisk(selfDisk, inode.Indirect, pointers.Data);

        // adjust offsetedDataBlock to start at 0 to account for zero-start
        // at the pointer data block
        size_t pointer = offsetedDataBlock - 5;
        while (read < length && pointer < POINTERS_PER_BLOCK && pointer < superBlock->Super.Blocks)
        {
            if (pointers.Pointers[pointer] == 0)
                break;
            selfDisk->readDisk(selfDisk, pointers.Pointers[pointer], block.Data);
            strncpy(data + read, block.Data + offset, fmin(BLOCK_SIZE, length - read));
            read += strlen(data + read);
            // fprintf(stderr, "====> Read: %d | %d | LEN %d | Offset:%d n", read, fmin(BLOCK_SIZE, length - read), strlen(block.Data), offset);
            offset = 0;
            pointer++;
        }
    }

    return read;
}

// Write to inode --------------------------------------------------------------

ssize_t writeInode(size_t inumber, char *data, size_t length, size_t offset)
{
    // printf("writeInode(inumber:%d, len:%d, offset:%d)\n", inumber, length, offset);
    // Load inode
    Inode inode;
    if (!loadInode(inumber, &inode))
    {
        return -1;
    }

    // Write block and copy to data
    Block block;
    uint32_t written = 0, freeblk;
    size_t offsetedDataBlock = offset / BLOCK_SIZE;
    offset %= BLOCK_SIZE;

    while (written < length && offsetedDataBlock < POINTERS_PER_INODE)
    {
        freeblk = allocFreeBlock(inode.Direct[offsetedDataBlock]);
        if (freeblk <= 0)
            return -1;
        inode.Direct[offsetedDataBlock] = freeblk;

        if (offset > 0)
        {
            selfDisk->readDisk(selfDisk, freeblk, block.Data);
        }
        strncpy(block.Data + offset, data + written, fmin(BLOCK_SIZE, length - written));

        selfDisk->writeDisk(selfDisk, freeblk, block.Data);
        written += strlen(block.Data);

        bzero(block.Data, BLOCK_SIZE);

        offset = 0;
        offsetedDataBlock++;
    }
    // printf("Ended writting to direct...\n");
    // printf("Wrote %d bytes of %d...\n", written, length);

    // still data to write,
    // use indirect data
    if (written < length)
    {
        // printf("Use indirect...\n");
        uint32_t indblk = allocFreeBlock(inode.Indirect);
        if (indblk <= 0)
        {
            return -1;
        }
        // printf("indirect data block: %d\n", indblk);
        inode.Indirect = indblk;

        // adjust offsetedDataBlock to start at 0 to account for zero-start
        // at the pointer data block
        offsetedDataBlock -= 5;

        Block pointers;
        selfDisk->readDisk(selfDisk, indblk, pointers.Data);
        while (written < length && offsetedDataBlock < POINTERS_PER_BLOCK)
        {
            freeblk = allocFreeBlock(pointers.Pointers[offsetedDataBlock]);
            if (freeblk <= 0)
                return -1;
            pointers.Pointers[offsetedDataBlock] = freeblk;

            if (offset > 0)
            {
                selfDisk->readDisk(selfDisk, freeblk, block.Data);
            }
            strncpy(block.Data + offset, data + written, fmin(BLOCK_SIZE, length - written));
            selfDisk->writeDisk(selfDisk, freeblk, block.Data);
            written += strlen(block.Data);
            bzero(block.Data, BLOCK_SIZE);

            offset = 0;
            offsetedDataBlock++;
        }

        selfDisk->writeDisk(selfDisk, indblk, pointers.Data);
    }

    // printf("Saving inode...\n"); // debug
    inode.Size += written;
    saveInode(inumber, &inode);
    // printf("Saved inode...\n");

    return written;
}
