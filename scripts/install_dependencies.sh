# install dependencies
sudo apt install -y build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo libisl-dev qemu-system-x86

# create temporary directory and navigate into it
mkdir -p tmp && cd tmp

# download sources
wget https://ftp.gnu.org/gnu/gcc/gcc-12.2.0/gcc-12.2.0.tar.gz
wget https://ftp.gnu.org/gnu/binutils/binutils-2.39.tar.gz

# extract sources
tar -xvf gcc-12.2.0.tar.gz
tar -xvf binutils-2.39.tar.gz

# set constants
export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

# create build directories
mkdir -p build-binutils build-gcc

# build and install binutils
cd build-binutils
../binutils-2.39/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make -j$(nproc)
make install
cd ..

# build and install GCC
cd build-gcc
../gcc-12.2.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers --disable-hosted-libstdcxx
make -j$(nproc) all-gcc
make -j$(nproc) all-target-libgcc
make -j$(nproc) all-target-libstdc++-v3
make install-gcc
make install-target-libgcc
make install-target-libstdc++-v3
cd ..

# test if it worked
$PREFIX/bin/$TARGET-gcc --version
