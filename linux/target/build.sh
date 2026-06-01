#!/bin/bash

function usage {
  echo "$0 [OPTIONS]"
  echo "  -v        Verbose"
}

verbose=n

while getopts cdhvs option
  do
    case "${option}"
      in
	    v) verbose=y;;
        h) usage && exit 1;;
    esac
  done

echo Building the ITB

cd ../build

if [ $verbose == y ]; then
  bitbake bsp -c itb -vDDD
  exit 0
fi

bitbake bsp -c itb


