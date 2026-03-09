#!/bin/sh
curdir=$pwd
mydir="${0%/*}"

cd $mydir

if [ ! -d $mydir/pybind11 ]
then
git clone https://github.com/pybind/pybind11
fi

cd pybind11
git checkout v2.8.0
