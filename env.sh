#!/bin/sh
export IB_ROOT_DIR=$PWD
export BUILDDIR="$PWD/build"
export BBPATH=$BUILDDIR
PATH=$PATH:$PWD/scripts:$BUILDDIR/bitbake/bin

# Specify auxiliary layers for the -x component opt
export IB_AUX_LAYERS="meta-usr meta-torizon meta-so3 meta-qemu meta-atf"

# TODO: It would be nice to test for the presence of such and
# such compiler in meta-bsp
if ! test -z $IB_TOOLCHAIN_PATH
then
	PATH="$PATH:$IB_TOOLCHAIN_PATH"
fi

# Avoid setting uid/gid of the regular user
# when sourced again by the root user in deploy.sh
if test -z "$IB_UNPRIVILEDGED_USER_ID"
then
	IB_UNPRIVILEDGED_USER_ID=`id -u`
fi

if test -z "$IB_UNPRIVILEDGED_GROUP_ID"
then
	IB_UNPRIVILEDGED_GROUP_ID=`id -g`
fi

# If not set - set default list of preserved variables
# that are passed to bitbake
if test -z "$BB_ENV_PASSTHROUGH_ADDITIONS"
then
	BB_ENV_PASSTHROUGH_ADDITIONS='IB_TOOLCHAIN_PATH IB_UNPRIVILEDGED_USER_ID IB_UNPRIVILEDGED_GROUP_ID IB_ROOT_DIR'
	export BB_ENV_PASSTHROUGH_ADDITIONS
fi

export PATH
export IB_UNPRIVILEDGED_USER_ID
export IB_UNPRIVILEDGED_GROUP_ID

