#!/bin/bash

image-5-output() {
    cat <<EOF
SuperBlock:
    magic number is valid
    5 blocks
    1 inode blocks
    113 inodes
Inode 1:
    size: 965 bytes
    direct blocks: 2
2 disk block reads
0 disk block writes
EOF
}

image-20-output() {
    cat <<EOF
SuperBlock:
    magic number is valid
    20 blocks
    2 inode blocks
    226 inodes
Inode 2:
    size: 27160 bytes
    direct blocks: 3 4 5 6 7
    indirect block: 8
    indirect data blocks: 9 10
Inode 3:
    size: 9546 bytes
    direct blocks: 11 12 13
4 disk block reads
0 disk block writes
EOF
}

test-debug() {
    DISK=$1
    BLOCKS=$2
    OUTPUT=$3

    echo -n "Testing debug on $DISK ... "
    if diff -u <(./bin/sfssh $DISK $BLOCKS <<<debug 2> /dev/null) <($OUTPUT) > test.log; then
    	echo "Success"
    else
    	echo "Failure"
    	cat test.log
    fi
    rm -f test.log
}

test-debug data/image.di.5   5   image-5-output
test-debug data/image.di.20  20  image-20-output
