#!/bin/bash

green="\033[0;32m"
blue="\033[1;34m"
red="\033[1;31m"
yellow="\033[0;33m"
restore="\033[0m"

mem_mb=`awk '$1 == "MemTotal:" { print int($2 / 1024)}' /proc/meminfo`
mem_half=$[mem_mb / 2 - mem_mb / 2 % 100]
memfile_size=$[mem_half - 20]

echo -e $blue"- Memory: $mem_mb M, half: $mem_half M, memfile: $memfile_size M"$restore

create_dir()
{
    if ! [ -d /tmp/libhammer/disk ]; then
        echo -e $green"- creating directories..."$restore
        if ! [ -d /tmp/libhammer ]; then
            mkdir /tmp/libhammer
        fi
        mkdir /tmp/libhammer/disk
    fi
    cp ./check_pa /tmp/libhammer/
    rm -f /tmp/libhammer/disk.img
}

create_ramdisk()
{
    if [ -e disk.img ]; then
        rm disk.img
    fi
    dd if=/dev/zero of=disk.img bs=1M count=$mem_half

    echo -e $green"- setting up loopback device"$restore
    losetup /dev/loop0 disk.img

    echo -e $green"- create/mount NTFS partition"$restore
    mkfs.ntfs /dev/loop0
    mount /dev/loop0 disk
    cd disk

    echo -e $green"- create eviction file..."$restore
  #  dd if=/dev/urandom of=memfile bs=1M count=$memfile_size
    dd if=/dev/zero of=memfile bs=1M count=$memfile_size seek=$memfile_size conv=sparse
    ls -l

    echo -e $green"- Done."$restore
}

remove_ramdisk()
{
    echo -e $yellow"- removing ramdisk..."$restore
    umount /dev/loop0
    losetup -d /dev/loop0
    rm /tmp/libhammer/disk.img
}

#------------------------------------
if [ a$1 = "aremove" ]; then
    remove_ramdisk
elif [ -e /tmp/libhammer/disk.img ]; then
    echo -e $yellow"- ramdisk already existing, use ./create_memfile remove to delete it."
else
    echo -e $green"- turn off swap"$restore
    swapoff -a
    create_dir
    pushd /tmp/libhammer
    create_ramdisk
    popd
fi
