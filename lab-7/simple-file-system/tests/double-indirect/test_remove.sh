#!/bin/bash

SCRATCH=$(mktemp -d)

# Test 0

test-0-input() {
    echo debug
    echo mount
    echo create
    echo create
    echo create
    echo remove 0
    echo debug
    echo create
    echo remove 0
    echo remove 0
    echo remove 1
    echo remove 3
    echo debug
}

test-0-output() {
    cat <<EOF
SuperBlock:
    magic number is valid
    5 blocks
    1 inode blocks
    113 inodes
Inode 1:
    size: 965 bytes
    direct blocks: 2
disk mounted.
created inode 0.
created inode 2.
created inode 3.
removed inode 0.
SuperBlock:
    magic number is valid
    5 blocks
    1 inode blocks
    113 inodes
Inode 1:
    size: 965 bytes
    direct blocks: 2
Inode 2:
    size: 0 bytes
    direct blocks:
Inode 3:
    size: 0 bytes
    direct blocks:
created inode 0.
removed inode 0.
remove failed!
removed inode 1.
removed inode 3.
SuperBlock:
    magic number is valid
    5 blocks
    1 inode blocks
    113 inodes
Inode 2:
    size: 0 bytes
    direct blocks:
21 disk block reads
8 disk block writes
EOF
}

cp data/image.di.5 $SCRATCH/image.di.5
trap "rm -fr $SCRATCH" INT QUIT TERM EXIT

echo -n "Testing remove in $SCRATCH/image.di.5 ... "
if diff -u <(test-0-input | ./bin/sfssh $SCRATCH/image.di.5 5 2> /dev/null) <(test-0-output) > $SCRATCH/test.log; then
    echo "Success"
else
    echo "False"
    cat $SCRATCH/test.log
fi

# Test 1

test-1-input() {
    cat <<EOF
debug
mount
copyout 1 $SCRATCH/1.txt
create
copyin $SCRATCH/1.txt 0
create
copyin $SCRATCH/1.txt 2
debug
remove 0
debug
create
copyin $SCRATCH/1.txt 0
debug
EOF
}

test-1-output() {
    cat <<EOF
SuperBlock:
    magic number is valid
    5 blocks
    1 inode blocks
    113 inodes
Inode 1:
    size: 965 bytes
    direct blocks: 2
disk mounted.
965 bytes copied
created inode 0.
965 bytes copied
created inode 2.
965 bytes copied
SuperBlock:
    magic number is valid
    5 blocks
    1 inode blocks
    113 inodes
Inode 0:
    size: 965 bytes
    direct blocks: 3
Inode 1:
    size: 965 bytes
    direct blocks: 2
Inode 2:
    size: 965 bytes
    direct blocks: 4
removed inode 0.
SuperBlock:
    magic number is valid
    5 blocks
    1 inode blocks
    113 inodes
Inode 1:
    size: 965 bytes
    direct blocks: 2
Inode 2:
    size: 965 bytes
    direct blocks: 4
created inode 0.
965 bytes copied
SuperBlock:
    magic number is valid
    5 blocks
    1 inode blocks
    113 inodes
Inode 0:
    size: 965 bytes
    direct blocks: 3
Inode 1:
    size: 965 bytes
    direct blocks: 2
Inode 2:
    size: 965 bytes
    direct blocks: 4
23 disk block reads
10 disk block writes
EOF
}

cp data/image.di.5 $SCRATCH/image.di.5
echo -n "Testing remove in $SCRATCH/image.di.5 ... "
if diff -u <(test-1-input | ./bin/sfssh $SCRATCH/image.di.5 5 2> /dev/null) <(test-1-output) > $SCRATCH/test.log; then
    echo "Success"
else
    echo "False"
    cat $SCRATCH/test.log
fi

# Test 2

test-2-input() {
    cat <<EOF
debug
mount
copyout 2 $SCRATCH/2.txt
remove 3
debug
create
copyin $SCRATCH/2.txt 0
debug
EOF
}

test-2-output() {
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
disk mounted.
27160 bytes copied
removed inode 3.
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
created inode 0.
27160 bytes copied
SuperBlock:
    magic number is valid
    20 blocks
    2 inode blocks
    226 inodes
Inode 0:
    size: 27160 bytes
    direct blocks: 11 12 13 14 15
    indirect block: 16
    indirect data blocks: 17 18
Inode 2:
    size: 27160 bytes
    direct blocks: 3 4 5 6 7
    indirect block: 8
    indirect data blocks: 9 10
34 disk block reads
11 disk block writes
EOF
}

cp data/image.di.20 $SCRATCH/image.di.20
echo -n "Testing remove in $SCRATCH/image.di.20 ... "
if diff -u <(test-2-input | ./bin/sfssh $SCRATCH/image.di.20 20 2> /dev/null) <(test-2-output) > $SCRATCH/test.log; then
    echo "Success"
else
    echo "False"
    cat $SCRATCH/test.log
fi
