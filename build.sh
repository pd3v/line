#!/bin/bash

mkdir build
cd build

# Not using JACK
cmake -DCMAKE_BUILD_TYPE=Release -DRTMIDI_API_JACK=OFF -DRTAUDIO_API_JACK=OFF -DLINK_BUILD_JACK=OFF -DBUILD_TESTING=OFF -DRTMIDI_BUILD_TESTING=OFF  ..

# Using JACK
#cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DRTMIDI_BUILD_TESTING=OFF ..

make