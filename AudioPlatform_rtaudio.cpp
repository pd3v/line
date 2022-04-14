/* Copyright 2016, Ableton AG, Berlin. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  If you would like to incorporate Link into a proprietary software application,
 *  please contact <link-devs@ableton.com>.
 */

#include "AudioPlatform_rtaudio.hpp"
#include <chrono>
#include <iostream>

typedef signed short MY_TYPE;
#define FORMAT RTAUDIO_SINT16
#define SCALE  32767.0

namespace ableton
{
namespace linkaudio
{

AudioPlatform::AudioPlatform(Link& link)
  : mEngine(link)
  , mSampleTime(0.)
{
  mEngine.setSampleRate(44100.);
  mEngine.setBufferSize(512);
  initialize();
  start();
}

AudioPlatform::~AudioPlatform() {
  stop();
}

//RtAudio
int AudioPlatform::audioCallback( void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,double streamTime, RtAudioStreamStatus statusFlags, void *userData ) {
  using namespace std::chrono;
  float* buffer = static_cast<float*>(outputBuffer);
  AudioPlatform& platform = *static_cast<AudioPlatform*>(userData);
  AudioEngine& engine = platform.mEngine;

  const auto hostTime = platform.mHostTimeFilter.sampleTimeToHostTime(platform.mSampleTime);

  platform.mSampleTime += static_cast<double>(nBufferFrames);

  const auto bufferBeginAtOutput = hostTime + engine.mOutputLatency.load();

  engine.audioCallback(bufferBeginAtOutput, nBufferFrames);

  for (unsigned long i = 0; i < nBufferFrames; ++i)
  {
    buffer[i * 2] = static_cast<float>(engine.mBuffer[i]);
    buffer[i * 2 + 1] = static_cast<float>(engine.mBuffer[i]);
  }

  return 0;
}

void AudioPlatform::initialize() {
  // RtAudio dac;
  if ( dac.getDeviceCount() == 0 ) exit( 0 );
  RtAudio::StreamParameters parameters;
  parameters.deviceId = dac.getDefaultOutputDevice();
  parameters.nChannels = 2;
  unsigned int sampleRate = mEngine.mSampleRate; //44100
  unsigned int bufferFrames = mEngine.mBuffer.size(); // 256 sample frames
  RtAudio::StreamOptions options;
  options.flags = RTAUDIO_NONINTERLEAVED;
  try {
    dac.openStream(&parameters, NULL, RTAUDIO_FLOAT32,
                    sampleRate, &bufferFrames, &audioCallback, this, &options, NULL/*&errorCallback*/);
  }
  catch ( RtAudioError& e ) {
    std::cout << '\n' << e.getMessage() << '\n' << std::endl;
    exit( 0 );
  }
}

// RtAudio does not use this
/*
void AudioPlatform::uninitialize()
{
  PaError result = Pa_CloseStream(pStream);
  if (result)
  {
    std::cerr << "Could not close Audio Stream. " << result << std::endl;
  }
  Pa_Terminate();

  if (!pStream)
  {
    std::cerr << "No valid audio stream." << std::endl;
    std::terminate();
  }
}
*/

void AudioPlatform::start() {
  try {
    dac.startStream();
  }
  catch ( RtAudioError& e ) {
    e.printMessage();
    exit(0);
  }
}


void AudioPlatform::stop()
{
  /*
  if (pStream == nullptr)
  {
    return;
  }

  PaError result = Pa_StopStream(pStream);
  if (result)
  {
    std::cerr << "Could not stop Audio Stream. " << result << std::endl;
    std::terminate();
  }
  */

 try {
    // Stop the stream
    dac.stopStream();
  }
  catch (RtAudioError& e) {
    e.printMessage();
  }
  if ( dac.isStreamOpen() ) dac.closeStream();

}

} // namespace linkaudio
} // namespace ableton
