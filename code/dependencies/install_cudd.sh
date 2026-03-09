#!/bin/sh

# Capture the current working directory
curdir=$(pwd)

# Get the directory of the script
mydir=$(dirname "$0")
cd "$mydir" || exit 1

# Clone the CUDD repository if it doesn't already exist
if [ ! -d cudd ]; then
    git clone https://github.com/ivmai/cudd.git || exit 1
fi

# Navigate to the CUDD directory
cd cudd || exit 1

# Generate build scripts if not already present
if [ ! -f configure ]; then
    aclocal -I . || exit 1
    autoheader || exit 1
    autoconf || exit 1
    automake --add-missing -c || exit 1
fi

# Configure the build
./configure --enable-obj --enable-shared --enable-dddmp --prefix="$curdir/cudd" || exit 1

# Build, test, and install
make -j 5 || exit 1
make check || exit 1
make install || exit 1

# Return to the original directory
cd "$curdir" || exit 1
