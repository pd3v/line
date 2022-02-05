#!/bin/bash

mkdir build
cd build

# Not using JACK
cmake -DCMAKE_BUILD_TYPE=Release -DRTMIDI_API_JACK=OFF -DRTMIDI_BUILD_TESTING=OFF -DRTMIDI_BUILD_SHARED_LIBS=OFF ..

# Using JACK
# cmake -DRTMIDI_BUILD_TESTING=OFF ..

make