#!/bin/bash

mkdir build
cd build

# Not using JACK
# cmake -DCMAKE_BUILD_TYPE=Release -DRTMIDI_API_JACK=OFF -DRTAUDIO_API_JACK=OFF -DRTAUDIO_API_JACK=OFF -DRTMIDI_BUILD_TESTING=OFF  ..

# Using ALSA
# cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DRTMIDI_BUILD_TESTING=OFF ..
cmake -DCMAKE_BUILD_TYPE=Release -DRTMIDI_API_JACK=ON -DBUILD_TESTING=OFF -DRTMIDI_BUILD_TESTING=OFF ..
 
make

# Remove unnecessary files after build done
# rm -r externals/ 
# rm -rf CMakeLists.txt build.sh line.cpp
