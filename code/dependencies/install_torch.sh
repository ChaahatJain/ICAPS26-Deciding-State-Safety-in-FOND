#!/bin/sh
curdir=$pwd
mydir="${0%/*}"

cd $mydir

if [[ $OSTYPE == 'darwin'* ]]; then
  download_path=https://download.pytorch.org/libtorch/cpu/libtorch-macos-1.12.1.zip
  zip_file=libtorch-macos-1.12.1.zip
else
  download_path=https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-1.8.1%2Bcpu.zip
  zip_file=libtorch-cxx11-abi-shared-with-deps-1.8.1+cpu.zip
fi

if [ ! -d "$mydir/torch" ]; then
    echo "==> Downloading libtorch..."
    wget -q -O "$mydir/$zip_file" "$download_path"

    echo "==> Unzipping libtorch..."
    unzip -q "$mydir/$zip_file" -d "$mydir"

    echo "==> Cleaning up..."
    rm -f "$mydir/$zip_file"

    echo "==> libtorch installed in $mydir"
else
    echo "==> libtorch already exists, skipping download"
fi
