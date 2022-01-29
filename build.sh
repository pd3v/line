#!/bin/bash

mkdir build
cd build

# Not using JACK
cmake -DRTMIDI_API_JACK=OFF RTMIDI_BUILD_TESTING=OFF ..

# Using JACK
# cmake -DRTMIDI_BUILD_TESTING=OFF ..

make