#!/bin/sh
curdir=$pwd
mydir="${0%/*}"

cd $mydir

if [ ! -d $mydir/cxxtest ]
then
git clone https://github.com/CxxTest/cxxtest.git
fi

cd cxxtest
git checkout 4.4
