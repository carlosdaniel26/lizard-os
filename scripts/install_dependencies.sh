# This script installs the dependencies required to build the GCC compiler.

sudo apt install build-essential &&
sudo apt install bison &&
sudo apt install flex &&
sudo apt install libgmp3-dev &&
sudo apt install libmpc-dev  &&
sudo apt install libmpfr-dev &&
sudo apt install texinfo &&
sudo apt install libisl-dev  &&

mkdir tmp
cd tmp

# download sources
wget https://ftp.gnu.org/gnu/gcc/gcc-12.2.0/gcc-12.2.0.tar.gz
wget https://ftp.gnu.org/gnu/binutils/binutils-2.39.tar.gz

# extract sources
tar -xvf gcc-12.2.0.tar.gz
tar -xvf binutils-2.39.tar.gz

# constants 
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

# build dir
mkdir build-binutils
mkdir build-gcc

# build binutils
../binutils-2.39/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make -j4
make install

# build gcc
../gcc-12.2.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers --disable-hosted-libstdcxx
make all-gcc
make all-target-libgcc
make all-target-libstdc++-v3
make install-gcc
make install-target-libgcc
make install-target-libstdc++-v3

# test if it worked
$HOME/opt/cross/bin/$TARGET-gcc --version
export PATH="$HOME/opt/cross/bin:$PATH"