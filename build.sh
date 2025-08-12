export TOOLCHAIN=$HOME/toolchain
export PATH=$PATH:$TOOLCHAIN/bin
export CC=$TOOLCHAIN/bin/arm-openwrt-linux-muslgnueabi-gcc
export CXX=$TOOLCHAIN/bin/arm-openwrt-linux-muslgnueabi-g++
export AR=$TOOLCHAIN/bin/arm-openwrt-linux-muslgnueabi-ar
export RANLIB=$TOOLCHAIN/bin/arm-openwrt-linux-muslgnueabi-ranlib
export STRIP=$TOOLCHAIN/bin/arm-openwrt-linux-muslgnueabi-strip

export STAGING_DIR=$TOOLCHAIN/bin

make -j${nproc}
