#!/bin/bash

image-5-input() {
    cat <<EOF
mount
stat 1
stat 2
stat 3
EOF
}

image-5-output() {
    cat <<EOF
disk mounted.
inode 1 has size 965 bytes.
stat failed!
stat failed!
3 disk block reads
0 disk block writes
EOF
}

image-20-input() {
    cat <<EOF
mount
stat 1
stat 2
stat 3
EOF
}

image-20-output() {
    cat <<EOF
disk mounted.
stat failed!
inode 2 has size 27160 bytes.
inode 3 has size 9546 bytes.
6 disk block reads
0 disk block writes
EOF
}

test-stat() {
    BLOCKS=$1

    echo -n "Testing stat on data/image.di.$BLOCKS... "
    if diff -u <(image-$BLOCKS-input | ./bin/sfssh data/image.di.$BLOCKS $BLOCKS 2> /dev/null) <(image-$BLOCKS-output) > test.log; then
    	echo "Success"
    else
    	echo "Failure"
    	cat test.log
    fi
    rm -f test.log
}

test-stat 5
test-stat 20
