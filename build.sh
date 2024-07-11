#!/bin/bash

mkdir build
cd build

# Not using JACK
cmake -DCMAKE_BUILD_TYPE=Release -DRTMIDI_API_JACK=OFF -DRTMIDI_BUILD_TESTING=OFF ..

# Using JACK
# cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DRTMIDI_BUILD_TESTING=OFF ..

make

# Remove unnecessary files after build done
# rm -r externals/ 
# rm -rf CMakeLists.txt build.sh line.cpp
