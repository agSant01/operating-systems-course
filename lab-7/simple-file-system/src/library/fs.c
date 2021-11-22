// fs.cpp: File System
#include "sfs/fs.h"

// #include <algorithm>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

ushort *freeblkmap;
ushort *inodetable;
Disk *selfDisk;
Block superBlock;

/**
 *  Cache-like implementation for retrieving most recent accessed INODE
 */
#define CACHE_LEN 3

Inode cached_inodes[CACHE_LEN] = {0};
size_t cached_inums[CACHE_LEN] = {0};

ushort victim = 0;
ushort referenceBits[CACHE_LEN] = {0};
ushort filled = 0;

/**
 * @brief Stores the instance of valid iNode in "inode" if there is a cache hit
 *
 * @param inumber inode number
 * @param inode inode struct to store variable
 * @return bool
 */
bool cacheLookup(size_t inumber, Inode *inode) {
    for (ushort i = 0; i < filled; i++) {
        if (cached_inums[i] == inumber) {
            referenceBits[i] = 1;
            memcpy(inode, &cached_inodes[i], sizeof(Inode));
            fprintf(stderr, "Cache hit: %lu\n", inumber);
            return true;
        }
    }

    memset(inode, 0, sizeof(Inode));
    return false;
}

bool cacheUpdate(size_t inumber, Inode *inode) {
    // check if exists in cache and update
    for (ushort i = 0; i < filled; i++) {
        if (cached_inums[i] == inumber) {
            referenceBits[i] = 1;
            memcpy(&cached_inodes[i], inode, sizeof(Inode));
            return true;
        }
    }

    // inode fault, not exists.
    if (filled == CACHE_LEN) {
        // find victim
        while (true) {
            if (referenceBits[victim] == 0) {
                memcpy(&cached_inodes[victim], inode, sizeof(Inode));
                cached_inums[victim] = inumber;
                break;
            }
            referenceBits[victim] = 0;
            victim = (victim + 1) % CACHE_LEN;
        }
    } else {
        memcpy(&cached_inodes[filled], inode, sizeof(Inode));
        cached_inums[filled] = inumber;
        filled++;
    }

    return true;
}

bool hasDiskMounted() {
    return selfDisk != NULL && selfDisk->mounted(selfDisk);
}

int gw(int a) { return log10((double)a) + 1; }

void printBitmaps() {
    if (!hasDiskMounted()) {
        printf("Must mount disk first.\n");
        return;
    }

    int max_;
    printf("\n------------------- Inodes -------------------\n");
    max_ = gw(superBlock.Super.Inodes) + 1;
    for (int i = 0; i < superBlock.Super.Inodes; i++) {
        printf("%d", i);
        if (i > 0) {
            printf("%-*s", max_ - gw(i), " ");
        } else {
            printf("%-*s", max_ - 1, " ");
        }
        printf("=> %d\n", inodetable[i]);
    }

    printf("------------------- Blocks -------------------\n");
    max_ = gw(superBlock.Super.Blocks) + 1;
    for (int i = 0; i < superBlock.Super.Blocks; i++) {
        printf("%d", i);
        if (i > 0) {
            printf("%-*s", max_ - gw(i), " ");
        } else {
            printf("%-*s", max_ - 1, " ");
        }
        printf("=> %d\n", freeblkmap[i]);
    }
}

void initIndirectBlocks(int indirectBlock) {
    Block indblock;
    selfDisk->readDisk(selfDisk, indirectBlock, indblock.Data);
    for (size_t pointer = 0;
         pointer < POINTERS_PER_BLOCK && pointer < selfDisk->Blocks;
         pointer++) {
        if (indblock.Pointers[pointer] != FREE) {
            freeblkmap[indblock.Pointers[pointer]] = OCCUPIED;
        }
    }
}

bool initFreeBlocks(Inode *inode) {
    if (inode->Valid == FREE) {
        return true;
    }

    for (ushort direct = 0; direct < POINTERS_PER_INODE; direct++) {
        if (inode->Direct[direct] != FREE) {
            freeblkmap[inode->Direct[direct]] = OCCUPIED; // mark data block as used
        }
    }

    if (inode->Indirect != FREE) {
        fprintf(stderr, "Inode indirect: %d\n", inode->Indirect);
        freeblkmap[inode->Indirect] = OCCUPIED;
        initIndirectBlocks(inode->Indirect);
    }

    if (inode->DoubleIndirect != FREE) {
        freeblkmap[inode->DoubleIndirect] = OCCUPIED;
        Block doubleIndBlk;
        selfDisk->readDisk(selfDisk, inode->DoubleIndirect, doubleIndBlk.Data);
        for (size_t pointer = 0;
             pointer < POINTERS_PER_BLOCK && pointer < superBlock.Super.Blocks;
             pointer++) {
            if (doubleIndBlk.Pointers[pointer] != FREE)
                initIndirectBlocks(doubleIndBlk.Pointers[pointer]);
        }
    }

    return true;
}

/**
 * @brief Validate if input bnumber is valid and return newly allocated block
 * number if input bnumber is "0". If free block is not available, return "-1"
 *
 * @param bnumber block number to compare, if "0" return new allocated block, if
 * "non-zero" return "bnumber"
 * @return ssize_t
 */
ssize_t allocFreeBlock(uint32_t bnumber) {
    if (bnumber != 0 && bnumber < superBlock.Super.Blocks) return bnumber;

    for (uint32_t i = superBlock.Super.InodeBlocks;
         i < superBlock.Super.Blocks;
         i++) {
        if (freeblkmap[i] == FREE) {
            freeblkmap[i] = OCCUPIED;
            return i;
        }
    }

    return -1;
}

bool initInodeTable() {
    free(inodetable);
    inodetable = calloc(superBlock.Super.Inodes, sizeof(ushort));

    free(freeblkmap);
    freeblkmap = calloc(superBlock.Super.Blocks, sizeof(ushort));
    freeblkmap[0] = OCCUPIED; // mark super block as used

    uint32_t inodeperblk =
        superBlock.Super.Inodes / superBlock.Super.InodeBlocks;
    Block block;
    for (size_t inodeblk = 1; inodeblk <= superBlock.Super.InodeBlocks;
         inodeblk++) {
        selfDisk->readDisk(selfDisk, inodeblk, block.Data);
        freeblkmap[inodeblk] = OCCUPIED;
        for (size_t inode = 0; inode < inodeperblk; inode++) {
            if (block.Inodes[inode].Valid == OCCUPIED) {
                inodetable[(inodeblk - 1) * inodeperblk + inode] = OCCUPIED;
                initFreeBlocks(&block.Inodes[inode]);
            }
        }
        fprintf(stderr, "InodeBlk: %lu\n", inodeblk);
    }

    fprintf(stderr, "Total InodeBlocks %u\n", superBlock.Super.InodeBlocks);

    return true;
}

ssize_t allocFreeInode() {
    size_t tinodes = superBlock.Super.Inodes;
    for (int i = 0; i < tinodes; i++) {
        if (inodetable[i] == FREE) {
            inodetable[i] = OCCUPIED;
            return i;
        }
    }

    return -1;
}

bool loadInode(size_t inumber, Inode *inode) {
    if (!hasDiskMounted()) {
        return false;
    }

    if (inumber < 0 || inumber >= superBlock.Super.Inodes) {
        return false;
    }

    if (inodetable[inumber] == FREE) {
        return false;
    }

    /* Cache lookup */
    if (cacheLookup(inumber, inode)) return true;

    /* Cache miss. Read inode from memory */
    size_t inodeperblk =
        superBlock.Super.Inodes / superBlock.Super.InodeBlocks;
    size_t blockNumber = (inumber / inodeperblk) + 1;
    Block block;
    selfDisk->readDisk(selfDisk, blockNumber, block.Data);
    memcpy(inode, &block.Inodes[inumber % inodeperblk], sizeof(Inode));

    /* Update inode Cache references */
    cacheUpdate(inumber, inode);

    return true;
}

bool saveInode(size_t inumber, Inode *inode) {
    if (!hasDiskMounted()) return false;

    if (inumber < 0 || inumber >= superBlock.Super.Inodes) {
        return false;
    }

    uint32_t inodesperblk =
        superBlock.Super.Inodes / superBlock.Super.InodeBlocks;
    uint32_t blockNumber = (inumber / inodesperblk) + 1;

    Block block;
    selfDisk->readDisk(selfDisk, blockNumber, block.Data);

    memcpy(&block.Inodes[inumber % inodesperblk], inode, sizeof(Inode));

    selfDisk->writeDisk(selfDisk, blockNumber, block.Data);

    cacheUpdate(inumber, inode);

    return true;
}

// Debug file system -----------------------------------------------------------

void debug(Disk *disk) {
    Block block;

    // Read Superblock
    disk->readDisk(disk, 0, block.Data);

    printf("SuperBlock:\n");

    if (block.Super.MagicNumber == MAGIC_NUMBER) {
        printf("    magic number is valid\n");
    } else {
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
    while (iblock <= totalIBlocks) {
        disk->readDisk(disk, iblock, block.Data);
        inodeIdx = 0;

        while (inodeIdx < INODES_PER_BLOCK) {
            // printf("IBlock: %d\n", inodeIdx); // debug
            inode = block.Inodes[inodeIdx];
            if (inode.Valid == OCCUPIED) {
                printf("Inode %d:\n", iblock * inodeIdx);
                printf("    size: %u bytes\n", inode.Size);

                printf("    direct blocks:");
                for (ushort idx = 0; idx < POINTERS_PER_INODE; idx++) {
                    if (inode.Direct[idx] == 0) break;
                    printf(" %u", inode.Direct[idx]);
                }
                printf("\n");

                if (inode.Indirect != FREE) {
                    Block indirect;
                    disk->readDisk(disk, inode.Indirect, indirect.Data);
                    printf("    indirect block: %u\n", inode.Indirect);
                    printf("    indirect data blocks:");
                    for (size_t ind = 0; ind < POINTERS_PER_BLOCK; ind++) {
                        if (indirect.Pointers[ind] == 0) break;
                        printf(" %u", indirect.Pointers[ind]);
                    }
                    printf("\n");
                }

                if (inode.DoubleIndirect != FREE) {
                    printf("    double indirect block: %u\n", inode.DoubleIndirect);
                    printf("    doubly directed indirect data blocks:");

                    Block doubleIndirect;
                    disk->readDisk(disk, inode.Indirect, doubleIndirect.Data);
                    for (size_t ind = 0; ind < POINTERS_PER_BLOCK; ind++) {
                        if (doubleIndirect.Pointers[ind] == 0) break;
                        printf(" %u", doubleIndirect.Pointers[ind]);
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

bool format(Disk *disk) {
    if (hasDiskMounted()) {
        return false;
    }

    Block block = {0};

    uint32_t inodeBlocks = (uint32_t)ceil(0.10 * (double)disk->Blocks);

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
    while (blockIdx < disk->Blocks) {
        disk->writeDisk(disk, blockIdx++, block.Data);
    }

    return true;
}

// Mount file system -----------------------------------------------------------

bool mount(Disk *disk) {
    if (disk->mounted(disk)) {
        return false;
    }

    // Read superblock
    disk->readDisk(disk, 0, superBlock.Data);
    if (superBlock.Super.MagicNumber != MAGIC_NUMBER) {
        fprintf(stderr, "Invalid valid magic number: %x...\n",
                superBlock.Super.MagicNumber); // debug
        return false;
    }
    fprintf(stderr, "Valid magic number: %x...\n",
            superBlock.Super.MagicNumber); // debug

    if (superBlock.Super.Blocks == 0) return false;

    if (superBlock.Super.InodeBlocks != ceil(0.10 * superBlock.Super.Blocks))
        return false;

    uint32_t inodes = superBlock.Super.Inodes;
    if (inodes == 0 || inodes % INODES_PER_BLOCK != 0) return false;

    // Set device
    selfDisk = disk;

    // Allocate free block freeblkmap
    // Allocate inode table
    // Copy metadata
    if (!initInodeTable()) return false;

    // Mount
    disk->mount(disk);

    return true;
}

// Create inode ----------------------------------------------------------------

ssize_t create() {
    if (!hasDiskMounted()) {
        return -1;
    }

    // Locate free inode in inode table
    ssize_t inodeidx = allocFreeInode();
    if (inodeidx < 0) return -1;

    size_t inodeperblk =
        superBlock.Super.Inodes / superBlock.Super.InodeBlocks;
    size_t bnumber = (inodeidx / inodeperblk) + 1;

    // Record inode if found
    Block block;
    selfDisk->readDisk(selfDisk, bnumber, block.Data);

    size_t adjustedinumber = inodeidx % inodeperblk;

    bzero(&block.Inodes[adjustedinumber], sizeof(Inode));
    block.Inodes[adjustedinumber].Valid = OCCUPIED;

    saveInode(inodeidx, &block.Inodes[adjustedinumber]);

    return inodeidx;
}

// Remove inode ----------------------------------------------------------------

void freeIndirectBlock(int indirectBlock) {
    Block indirectBlk;
    selfDisk->readDisk(selfDisk, indirectBlock, indirectBlk.Data);

    for (size_t pointer = 0;
         pointer < POINTERS_PER_BLOCK && pointer < superBlock.Super.Blocks;
         pointer++) {
        // printf("pointer: %d...\n", pointer);
        if (indirectBlk.Pointers[pointer] == FREE) continue;
        freeblkmap[indirectBlk.Pointers[pointer]] = FREE;
        indirectBlk.Pointers[pointer] = FREE;
    }
    selfDisk->writeDisk(selfDisk, indirectBlock, indirectBlk.Data);

    freeblkmap[indirectBlock] = FREE;
}

bool removeInode(size_t inumber) {
    // Load inode information
    Inode inode;
    if (!loadInode(inumber, &inode)) {
        return false;
    }

    // Clear inode in inode table
    inodetable[inumber] = FREE;
    inode.Valid = FREE;
    inode.Size = 0;

    // Free direct blocks
    for (int direct = 0; direct < POINTERS_PER_INODE; direct++) {
        if (inode.Direct[direct] == FREE) continue;
        freeblkmap[inode.Direct[direct]] = FREE;
        inode.Direct[direct] = FREE;
    }
    // printf("freed direct blocks...\n");

    // Free indirect blocks
    if (inode.Indirect != FREE) {
        freeIndirectBlock(inode.Indirect);
        inode.Indirect = FREE;
    }

    if (inode.DoubleIndirect != FREE) {
        Block doubleIndirect;
        selfDisk->readDisk(selfDisk, inode.DoubleIndirect, doubleIndirect.Data);
        for (size_t pointer = 0;
             pointer < POINTERS_PER_BLOCK && pointer < superBlock.Super.Blocks;
             pointer++) {
            if (doubleIndirect.Pointers[pointer] != FREE)
                freeIndirectBlock(doubleIndirect.Pointers[pointer]);
        }
        inode.DoubleIndirect = FREE;
    }
    fprintf(stderr, "freed indirect blocks...\n");

    saveInode(inumber, &inode);

    return true;
}

// Inode stat ------------------------------------------------------------------

ssize_t stat(size_t inumber) {
    if (!hasDiskMounted()) {
        fprintf(stderr, "Mount disk first...\n");
        return -1;
    }

    // Load inode information
    Inode inode;
    if (!loadInode(inumber, &inode)) {
        fprintf(stderr, "Invalid inode...\n");
        return -1;
    };

    return inode.Size;
}

// Read from inode -------------------------------------------------------------
void readFromIndirect(Block *pointers, size_t *pointer, char *data, size_t length, int *read, size_t *offset) {
    fprintf(stderr, "Entering readIndirect()...\n");
    Block block;
    while (*read < length && (*pointer) < POINTERS_PER_BLOCK && (*pointer) < superBlock.Super.Blocks) {
        if (pointers->Pointers[(*pointer)] == FREE ||
            freeblkmap[pointers->Pointers[(*pointer)]] == FREE)
            break;

        selfDisk->readDisk(selfDisk, pointers->Pointers[(*pointer)], block.Data);

        int lenData = strnlen(block.Data + *offset, BLOCK_SIZE);
        int maxCopy = fmin(lenData, length - *read);

        strncpy(data + *read, block.Data + *offset, maxCopy);
        bzero(block.Data, BLOCK_SIZE);

        (*read) += maxCopy;

        *offset = 0;
        (*pointer)++;
    }
    return;
}

ssize_t readInode(size_t inumber, char *data, size_t length, size_t offset) {
    fprintf(stderr, "readInode(inumber:%ld, length:%ld, offset:%ld)\n", inumber,
            length, offset);
    // Load inode information
    Inode inode;
    if (!loadInode(inumber, &inode)) {
        return -1;
    }
    // printf("Loaded inode...\n"); // debug

    // Adjust length
    size_t offsetedDataBlock = offset / BLOCK_SIZE;
    offset %= BLOCK_SIZE;
    Block block;
    bzero(block.Data, BLOCK_SIZE);
    int read = 0;

    // printf("StartBlock %d\n", startBlock); // debug
    while (offsetedDataBlock < POINTERS_PER_INODE && read < length) {
        if (freeblkmap[inode.Direct[offsetedDataBlock]] == FREE ||
            inode.Direct[offsetedDataBlock] == FREE)
            break;

        fprintf(stderr, "Hi %d\n", inode.Direct[offsetedDataBlock]);

        selfDisk->readDisk(selfDisk, inode.Direct[offsetedDataBlock],
                           block.Data);

        int lenData = strnlen(block.Data + offset, BLOCK_SIZE);
        int maxCopy = fmin(lenData, length - read);

        strncpy(data + read, block.Data + offset, maxCopy);
        bzero(block.Data, BLOCK_SIZE);

        read += maxCopy;

        fprintf(stderr, "Read: %d | Offset:%ld\n", read, offset);
        offset = 0;
        offsetedDataBlock++;
    }

    fprintf(stderr, "Ended reading directs...\n");
    if (inode.Indirect != FREE &&
        offsetedDataBlock < POINTERS_PER_BLOCK + POINTERS_PER_INODE &&
        read < length) {
        // if (inode.Indirect != FREE) {
        fprintf(stderr, "Started reading indirects...\n");

        Block pointers;
        selfDisk->readDisk(selfDisk, inode.Indirect, pointers.Data);

        // adjust offsetedDataBlock to start at 0 to account for zero-start
        // at the pointer data block
        size_t pointer = offsetedDataBlock - POINTERS_PER_INODE;
        readFromIndirect(&pointers, &pointer, data, length, &read, &offset);
        offsetedDataBlock = pointer + POINTERS_PER_INODE;
    }

    if (inode.DoubleIndirect != FREE && read < length) {
        fprintf(stderr, "Reading Double Indirect...\n");

        Block doubleIndirect;
        selfDisk->readDisk(selfDisk, inode.DoubleIndirect, doubleIndirect.Data);

        size_t pointer = offsetedDataBlock - POINTERS_PER_BLOCK - POINTERS_PER_INODE;
        size_t indirectBlockIdx = (pointer / POINTERS_PER_BLOCK);
        pointer %= POINTERS_PER_BLOCK;

        while (read < length && indirectBlockIdx < POINTERS_PER_BLOCK) {
            if (doubleIndirect.Pointers[indirectBlockIdx] == FREE ||
                freeblkmap[doubleIndirect.Pointers[indirectBlockIdx]] == FREE) break;
            // get indirect and read
            Block indirect;
            selfDisk->readDisk(selfDisk, indirectBlockIdx, indirect.Data);

            readFromIndirect(&indirect, &pointer, data, length, &read, &offset);

            offsetedDataBlock += pointer;
            indirectBlockIdx++;
            pointer = 0;
        }
    }

    return read;
}

// Write to inode --------------------------------------------------------------

void writeToIndirect(Block *pointers, size_t *pointer, char *data, size_t length, uint32_t *written, size_t *offset) {
    Block block;
    ssize_t freeblk;

    while (*written < length && *pointer < POINTERS_PER_BLOCK) {
        freeblk = allocFreeBlock(pointers->Pointers[*pointer]);

        if (freeblk <= 0) break;

        pointers->Pointers[*pointer] = freeblk;
        fprintf(stderr, "Free Blck: %ld\n", freeblk);

        if (*offset > 0) {
            selfDisk->readDisk(selfDisk, freeblk, block.Data);
        }

        int maxCopy = fmin(BLOCK_SIZE - *offset, length - *written);
        strncpy(block.Data + *offset, data + *written, maxCopy);

        selfDisk->writeDisk(selfDisk, freeblk, block.Data);
        bzero(block.Data, BLOCK_SIZE);

        *written += maxCopy;

        *offset = 0;
        (*pointer)++;
    }

    // mark the end of data
    if (*pointer < POINTERS_PER_BLOCK)
        pointers->Pointers[*pointer] = FREE;
}

ssize_t writeInode(size_t inumber, char *data, size_t length, size_t offset) {
    // printf("writeInode(inumber:%d, len:%d, offset:%d)\n", inumber, length,
    // offset); Load inode
    Inode inode;
    if (!loadInode(inumber, &inode)) {
        return -1;
    }

    // Write block and copy to data
    bool isDiskFull = false;
    Block block;
    uint32_t written = 0;
    ssize_t freeblk;
    size_t offsetedDataBlock = offset / BLOCK_SIZE;
    offset %= BLOCK_SIZE;

    fprintf(stderr, "Write on direct...\n");

    while (written < length && offsetedDataBlock < POINTERS_PER_INODE) {
        freeblk = allocFreeBlock(inode.Direct[offsetedDataBlock]);

        if (freeblk <= 0) {
            isDiskFull = true;
            break;
        };

        fprintf(stderr, "Free block: %ld\n", freeblk);
        inode.Direct[offsetedDataBlock] = freeblk;

        if (offset > 0) {
            selfDisk->readDisk(selfDisk, freeblk, block.Data);
        }
        int maxCopy = fmin(BLOCK_SIZE, length - written);
        strncpy(block.Data + offset, data + written, maxCopy);

        selfDisk->writeDisk(selfDisk, freeblk, block.Data);
        bzero(block.Data, BLOCK_SIZE);

        written += maxCopy;

        offset = 0;
        offsetedDataBlock++;
    }

    // mark the end of data
    if (offsetedDataBlock < POINTERS_PER_INODE)
        inode.Direct[offsetedDataBlock] = FREE;

    fprintf(stderr, "Ended writting to direct...\n");
    fprintf(stderr, "Wrote %u bytes of %lu...\n", written, length);

    // still data to write,
    // use indirect data
    if (written < length && !isDiskFull && offsetedDataBlock < POINTERS_PER_BLOCK + POINTERS_PER_INODE) {
        fprintf(stderr, "Use indirect...\n");
        ssize_t indblk = allocFreeBlock(inode.Indirect);

        if (indblk > 0) {
            inode.Indirect = indblk;

            fprintf(stderr, "Indirect block:%ld\n", indblk);

            Block pointers;
            selfDisk->readDisk(selfDisk, indblk, pointers.Data);

            // adjust offsetedDataBlock to start at 0 to account for zero-start
            // at the pointer data block
            offsetedDataBlock -= POINTERS_PER_INODE;
            writeToIndirect(&pointers, &offsetedDataBlock, data, length, &written, &offset);

            selfDisk->writeDisk(selfDisk, indblk, pointers.Data);
        }
    }

    // use double indirect block
    if (written < length) {
        fprintf(stderr, "Use double indirect...\n");
        ssize_t doubleIndirect = allocFreeBlock(inode.DoubleIndirect);

        if (doubleIndirect > 0) {
            Block indirectBlocks;
            selfDisk->readDisk(selfDisk, doubleIndirect, indirectBlocks.Data);

            size_t pointer = offsetedDataBlock - POINTERS_PER_INODE - POINTERS_PER_BLOCK;
            size_t indirectBlock = pointer / POINTERS_PER_BLOCK;
            pointer %= POINTERS_PER_BLOCK;

            while (written < length && indirectBlock < POINTERS_PER_BLOCK) {
                size_t indBlkAddr = allocFreeBlock(indirectBlocks.Pointers[indirectBlock]);

                if (indBlkAddr <= 0) break;

                indirectBlocks.Pointers[indirectBlock] = indBlkAddr;

                Block indBlock;
                selfDisk->readDisk(selfDisk, indBlkAddr, indBlock.Data);

                writeToIndirect(&indBlock, &pointer, data, length, &written, &offset);

                offsetedDataBlock += pointer;
                indirectBlock++;
                pointer = 0;

                selfDisk->writeDisk(selfDisk, indirectBlock, indBlock.Data);
            }

            selfDisk->writeDisk(selfDisk, doubleIndirect, indirectBlocks.Data);
        }
    }

    fprintf(stderr, "Saving inode...\n"); // debug
    inode.Size = written;
    saveInode(inumber, &inode);
    fprintf(stderr, "Saved inode...\n");

    return written;
}
