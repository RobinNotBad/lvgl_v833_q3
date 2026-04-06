export TOOLCHAIN=$HOME/toolchain-glibc
export PATH=$PATH:$TOOLCHAIN/bin
export CC=$TOOLCHAIN/bin/arm-openwrt-linux-gcc
export CXX=$TOOLCHAIN/bin/arm-openwrt-linux-g++
export AR=$TOOLCHAIN/bin/arm-openwrt-linux-ar
export RANLIB=$TOOLCHAIN/bin/arm-openwrt-linux-ranlib
export STRIP=$TOOLCHAIN/bin/arm-openwrt-linux-strip

export STAGING_DIR=$TOOLCHAIN/bin

make -j${nproc}
