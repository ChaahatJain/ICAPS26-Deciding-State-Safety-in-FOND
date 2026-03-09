#!/bin/sh
curdir=$pwd
mydir="${0%/*}"

cd $mydir

if [ ! -d $mydir/z3 ]
then
git clone https://github.com/Z3Prover/z3.git
fi

cd z3
git checkout z3-4.8.12

# back to dependencies to execute cmake script
cd ..
sh cmake_z3.sh

# build ...
cd z3/build
make -j 1 # local-machine-friendly j ...