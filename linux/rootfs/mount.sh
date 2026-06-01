#!/bin/bash

usage()
{
  echo "Usage: $0 {ramfs|rootfs}"
  exit 1
}

cd ../../build
source env.sh

if [[ $1 == "ramfs" ]]; then
    bitbake rootfs-linux -c ramfs_mount
    exit 0
fi

if [[ $1 == "rootfs" ]]; then
    bitbake rootfs-linux -c rootfs_mount
    exit 0
fi

usage



