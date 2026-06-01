#!/bin/bash

function usage {
  echo "$0 [OPTIONS]"
  echo "  -v        Verbose"
  echo "  -h        Print this help"
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


if [ $verbose == y ]; then
  cd ../build
  bitbake bsp -c do_itb -vDDD
  exit 0
fi

cd ../build
bitbake bsp -c do_itb




