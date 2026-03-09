#!/bin/sh

build_type=${1} # Release, Debug, ...
build_dir=${2}

if [ $# -gt 2 ]
then
dependencies=-DDEPENDENCIES_PATH=${3}
else
dependencies=${3}
fi

mkdir -p ${build_dir}
cd ${build_dir}
cmake -DCMAKE_BUILD_TYPE=${build_type} ${dependencies} -G "CodeBlocks - Unix Makefiles" ../
