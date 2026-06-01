#!/bin/bash

function usage {
  echo "$0 [OPTIONS]"
  echo "  -v        Verbose"
}

clean=n
verbose=n

while getopts v option
  do
    case "${option}"
      in
	    v) verbose=y;;
        h) usage && exit 1;;
    esac
  done

echo Now deploying rootfs in second partition

cd ../../build
source env.sh

if [ $verbose == y ]; then
  rm build/tmp/stamps/rootfs-linux*
  bitbake rootfs-linux -c deploy -vDDD
  exit 0
fi

bitbake rootfs-linux -c deploy
