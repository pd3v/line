#!/bin/bash

mkdir build
cd build

# Not using JACK
cmake -DCMAKE_BUILD_TYPE=Release -DRTMIDI_API_JACK=OFF -DRTMIDI_BUILD_TESTING=OFF ..

# Using JACK
# cmake -DCMAKE_BUILD_TYPE=Release -DRTMIDI_BUILD_TESTING=OFF ..

make
