#!/bin/sh
curdir=$pwd
mydir="${0%/*}"

cd $mydir

if [ ! -d $mydir/Marabou ]
then
git clone https://gitlab.cs.uni-saarland.de/fai/code/Marabou.git
fi

cd Marabou
git checkout icaps24Public

# Generate api ...
cd ./c++
python3 generate_c++_api.py
cd ..

# execute cmake script
sh ./cmake_marabou.sh

# build ...
cd build
make -j 4 MarabouHelper # local-machine-friendly j ...