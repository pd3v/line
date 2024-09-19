#!/bin/bash

mkdir build
cd build

# Not using JACK
cmake -Wall -DCMAKE_BUILD_TYPE=Release -DGIT_ACTION=ON -DBUILD_17=ON -DRTMIDI_API_JACK=OFF -DRTMIDI_BUILD_TESTING=OFF  ..

# Using JACK
# cmake -DCMAKE_BUILD_TYPE=Release -DRTMIDI_BUILD_TESTING=OFF ..

make

chmod +x line

# Remove unnecessary files after build done
# rm -r externals/ 
# rm -rf CMakeLists.txt build.sh line.cpp
