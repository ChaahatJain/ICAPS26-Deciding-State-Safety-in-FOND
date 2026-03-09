#!/bin/sh

cd z3
mkdir -p build
cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DZ3_SINGLE_THREADED=True ../
