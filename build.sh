export TOOLCHAIN=$HOME/toolchain-glibc
export PATH=$PATH:$TOOLCHAIN/bin
export CC=$TOOLCHAIN/bin/aarch64-openwrt-linux-gcc
export CXX=$TOOLCHAIN/bin/aarch64-openwrt-linux-g++
export AR=$TOOLCHAIN/bin/aarch64-openwrt-linux-ar
export RANLIB=$TOOLCHAIN/bin/aarch64-openwrt-linux-ranlib
export STRIP=$TOOLCHAIN/bin/aarch64-openwrt-linux-strip

export STAGING_DIR=$TOOLCHAIN/bin

make -j${nproc}
