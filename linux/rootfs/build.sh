#!/bin/bash

function usage {
  echo "$0 [OPTIONS]"
  echo "  -c        Clean"
  echo "  -v        Verbose"
}

clean=n
verbose=n

while getopts cv option
  do
    case "${option}"
      in
        c) clean=y;;
	    v) verbose=y;;
        h) usage && exit 1;;
    esac
  done

if [ $clean == y ]; then
  echo "Cleaning rootfs"
  cd ../../build
  bitbake rootfs-linux -c clean
  exit 0
fi

echo Building the rootfs

cd ../../build
source env.sh

if [ $verbose == y ]; then
  bitbake rootfs-linux -vDDD
  exit 0
fi

bitbake rootfs-linux


