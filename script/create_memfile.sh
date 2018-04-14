#!/bin/bash

mem_mb=`awk '$1 == "MemTotal:" { print int($2 / 1024)}' /proc/meminfo`
mem_half=$[mem_mb / 2 - mem_mb / 2 % 100]
memfile_size=$[mem_half - 20]

echo "Memory: $mem_mb M, half: $mem_half M, memfile: $memfile_size M"

if ! [ -d /tmp/libhammer ]; then
    mkdir /tmp/libhammer
fi
pushd

cd /tmp/libhammer
if ! [ -d disk ]; then
    mkdir disk
fi

dd if=/dev/zero of=disk.img bs=1M count=$mem_half
losetup /dev/loop0 disk.img
mkfs.ext4 /dev/loop0
mount /dev/loop0 disk
cd disk
dd if=/dev/urandom of=memfile bs=1M count=$memfile_size
ls -l

popd


