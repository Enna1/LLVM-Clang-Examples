#!/bin/bash

num_cores=1
target_dir=./
re_number="^[0-9]+$"

if [ "$#" -ne 2 ] || ! [[ "$1" =~ ${re_number} ]] || ! [ -d "$2" ]; then
	echo "usage: <prog> <# cores> <directory>" >&2
	exit 1
fi

num_cores=$1
target_dir=$2

echo "Getting the complete LLVM source code"
cd ${target_dir}
# download
echo "Get llvm"
wget http://releases.llvm.org/5.0.1/llvm-5.0.1.src.tar.xz
echo "Get clang"
wget http://releases.llvm.org/5.0.1/cfe-5.0.1.src.tar.xz
echo "Get clang-tools-extra"
wget http://releases.llvm.org/5.0.1/clang-tools-extra-5.0.1.src.tar.xz
echo "Get lld"
wget http://releases.llvm.org/5.0.1/lld-5.0.1.src.tar.xz
echo "Get polly"
wget http://releases.llvm.org/5.0.1/polly-5.0.1.src.tar.xz
echo "Get compiler-rt"
wget http://releases.llvm.org/5.0.1/compiler-rt-5.0.1.src.tar.xz
echo "Get openmp"
wget http://releases.llvm.org/5.0.1/openmp-5.0.1.src.tar.xz
echo "Get libcxx"
wget http://releases.llvm.org/5.0.1/libcxx-5.0.1.src.tar.xz
echo "Get libcxxabi"
wget http://releases.llvm.org/5.0.1/libcxxabi-5.0.1.src.tar.xz
echo "Get test-suite"
wget http://releases.llvm.org/5.0.1/test-suite-5.0.1.src.tar.xz

# extract & rename
tar xvJf ./llvm-5.0.1.src.tar.xz
mv ./llvm-5.0.1.src ./llvm-5.0.1
tar xvJf ./cfe-5.0.1.src.tar.xz -C ./llvm-5.0.1/tools
mv ./llvm-5.0.1/tools/cfe-5.0.1.src ./llvm-5.0.1/tools/clang
tar xvJf ./clang-tools-extra-5.0.1.src.tar.xz -C ./llvm-5.0.1/tools/clang/tools
mv ./llvm-5.0.1/tools/clang/tools/clang-tools-extra-5.0.1.src ./llvm-5.0.1/tools/clang/tools/extra
tar xvJf ./lld-5.0.1.src.tar.xz -C ./llvm-5.0.1/tools
mv ./llvm-5.0.1/tools/lld-5.0.1.src ./llvm-5.0.1/tools/lld
tar xvJf ./polly-5.0.1.src.tar.xz -C ./llvm-5.0.1/tools
mv ./llvm-5.0.1/tools/polly-5.0.1.src ./llvm-5.0.1/tools/polly
tar xvJf ./compiler-rt-5.0.1.src.tar.xz -C ./llvm-5.0.1/projects
mv ./llvm-5.0.1/projects/compiler-rt-5.0.1.src ./llvm-5.0.1/projects/compiler-rt
tar xvJf ./openmp-5.0.1.src.tar.xz -C ./llvm-5.0.1/projects
mv ./llvm-5.0.1/projects/openmp-5.0.1.src ./llvm-5.0.1/projects/openmp
tar xvJf ./libcxx-5.0.1.src.tar.xz -C ./llvm-5.0.1/projects
mv ./llvm-5.0.1/projects/libcxx-5.0.1.src ./llvm-5.0.1/projects/libcxx
tar xvJf ./libcxxabi-5.0.1.src.tar.xz -C ./llvm-5.0.1/projects
mv ./llvm-5.0.1/projects/libcxxabi-5.0.1.src ./llvm-5.0.1/projects/libcxxabi
tar xvJf ./test-suite-5.0.1.src.tar.xz -C ./llvm-5.0.1/projects
mv ./llvm-5.0.1/projects/test-suite-5.0.1.src ./llvm-5.0.1/projects/test-suite

# build binutils
cd ./llvm-5.0.1
echo "Get new-ld with plugin support"
git clone --depth 1 git://sourceware.org/git/binutils-gdb.git binutils
cd binutils
mkdir build
cd build
echo "build binutils"
sudo apt-get install texinfo bison flex
../configure --disable-werror
make -j${num_cores} all-ld

# build llvm & install
cd ../..
echo "LLVM source code and plugins are set up"
echo "Build the LLVM project"
mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_CXX1Y=ON -DLLVM_ENABLE_EH=ON -DLLVM_ENABLE_RTTI=ON -DLLVM_BUILD_LLVM_DYLIB=ON -DLLVM_BINUTILS_INCDIR=$(pwd)/../binutils/include ..
make -j${num_cores}
echo "Installing LLVM"
sudo make install
echo "Successfully installed LLVM"