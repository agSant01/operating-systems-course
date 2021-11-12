// disk.h: Disk emulator

#pragma once

#include <stdbool.h>
#include <stdlib.h>

// Number of bytes per block
#define BLOCK_SIZE 4096

typedef struct Disk {
    int FileDescriptor; // File descriptor of disk image
    size_t Blocks;      // Number of blocks in disk image
    size_t Reads;       // Number of reads performed
    size_t Writes;      // Number of writes performed
    size_t Mounts;      // Number of mounts

    // Check parameters
    // @param	blocknum    Block to operate on
    // @param	data	    Buffer to operate on
    void (*sanity_check)(struct Disk *self, int blocknum, char *data);

    // Destructor
    void (*DiskDestructor)(struct Disk *self);

    // Open disk image
    // @param	path	    Path to disk image
    // @param	nblocks	    Number of blocks in disk image
    void (*open)(struct Disk *self, const char *path, size_t nblocks);

    // Return size of disk (in terms of blocks)
    size_t (*size)(struct Disk *self);

    // Return whether or not disk is mounted
    bool (*mounted)(struct Disk *self);

    // Increment mounts
    void (*mount)(struct Disk *self);

    // Decrement mounts
    void (*unmount)(struct Disk *self);

    // Read block from disk
    // @param	blocknum    Block to read from
    // @param	data	    Buffer to read into
    void (*readDisk)(struct Disk *self, int blocknum, char *data);

    // Write block to disk
    // @param	blocknum    Block to write to
    // @param	data	    Buffer to write from
    void (*writeDisk)(struct Disk *self, int blocknum, char *data);
} Disk;
