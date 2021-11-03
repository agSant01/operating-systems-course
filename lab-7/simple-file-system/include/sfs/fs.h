// fs.h: File System

#pragma once

#include "sfs/disk.h"

#include <stdint.h>
#include <stdbool.h>

#define MAGIC_NUMBER 0xf0f03410
#define INODES_PER_BLOCK 128
#define POINTERS_PER_INODE 5
#define POINTERS_PER_BLOCK 1024

typedef struct SuperBlock
{                         // Superblock structure
    uint32_t MagicNumber; // File system magic number
    uint32_t Blocks;      // Number of blocks in file system
    uint32_t InodeBlocks; // Number of blocks reserved for inodes
    uint32_t Inodes;      // Number of inodes in file system
} SuperBlock;

typedef struct Inode
{
    uint32_t Valid;                      // Whether or not inode is valid
    uint32_t Size;                       // Size of file
    uint32_t Direct[POINTERS_PER_INODE]; // Direct pointers
    uint32_t Indirect;                   // Indirect pointer
    //uint32_t DoubleIndirect;	// Double Indirect pointer
} Inode;

typedef union Block
{
    SuperBlock Super;                      // Superblock
    Inode Inodes[INODES_PER_BLOCK];        // Inode block
    uint32_t Pointers[POINTERS_PER_BLOCK]; // Pointer block
    char Data[BLOCK_SIZE];                 // Data block
} Block;

typedef struct FileSystem
{

    void (*debug)(Disk *disk);
    bool (*format)(Disk *disk);

    bool (*mount)(Disk *disk);

    size_t (*create)();
    bool (*removeInode)(size_t inumber);
    size_t (*stat)(size_t inumber);

    size_t (*readInode)(size_t inumber, char *data, size_t length, size_t offset);
    size_t (*writeInode)(size_t inumber, char *data, size_t length, size_t offset);

} FileSystem;
