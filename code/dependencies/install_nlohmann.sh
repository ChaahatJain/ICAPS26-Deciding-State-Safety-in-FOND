#!/bin/sh
curdir=$pwd
mydir="${0%/*}"

cd $mydir

if [ ! -d $mydir/json ]
then
git clone https://github.com/nlohmann/json.git
fi

cd json
git checkout v3.11.2
