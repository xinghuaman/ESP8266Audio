/*
  AudioGeneratorWAV
  Audio output generator that reads 8 and 16-bit WAV files
  
  Copyright (C) 2017  Earle F. Philhower, III

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "AudioGeneratorWAV.h"

AudioGeneratorWAV::AudioGeneratorWAV()
{
  running = false;
  file = NULL;
  output = NULL;
  buffSize = 128;
  buff = NULL;
  buffPtr = 0;
  buffLen = 0;
}

AudioGeneratorWAV::~AudioGeneratorWAV()
{
  free(buff);
  buff = NULL;
}

bool AudioGeneratorWAV::stop()
{
  if (!running) return true;
  running = false;
  free(buff);
  buff = NULL;
  return file->close();
}

bool AudioGeneratorWAV::isRunning()
{
  return running;
}


// Handle buffered reading, reload each time we run out of data
bool AudioGeneratorWAV::GetBufferedData(int bytes, void *dest)
{
  if (!running) return false; // Nothing to do here!
  uint8_t *p = reinterpret_cast<uint8_t*>(dest);
  while (bytes--) {
    // Potentially load next batch of data...
    if (buffPtr >= buffLen) {
      buffPtr = 0;
      buffLen = file->read( buff, buffSize );
    }
    if (buffPtr >= buffLen)
      return false; // No data left!
    *(p++) = buff[buffPtr++];
  }
  return true;
}

bool AudioGeneratorWAV::loop()
{
  if (!running) return true; // Nothing to do here!

  // First, try and push in the stored sample.  If we can't, then punt and try later
  if (!output->ConsumeSample(lastSample)) return true; // Can't send, but no error detected

  // Try and stuff the buffer one sample at a time
  do
  {
    if (bitsPerSample == 8) {
      uint8_t l, r;
      if (!GetBufferedData(1, &l)) stop();
      if (channels == 2) {
        if (!GetBufferedData(1, &r)) stop();
      } else {
        r = 0;
      }
      lastSample[AudioOutput::LEFTCHANNEL] = l;
      lastSample[AudioOutput::RIGHTCHANNEL] = r;
    } else if (bitsPerSample == 16) {
      if (!GetBufferedData(2, &lastSample[AudioOutput::LEFTCHANNEL])) stop();
      if (channels == 2) {
        if (!GetBufferedData(2, &lastSample[AudioOutput::RIGHTCHANNEL])) stop();
      } else {
        lastSample[AudioOutput::RIGHTCHANNEL] = 0;
      }
    }
  } while (running && output->ConsumeSample(lastSample));

  return running;
}


bool AudioGeneratorWAV::ReadWAVInfo()
{
  uint32_t u32;
  uint16_t u16;

  // Header == "RIFF"
  if (!ReadU32(&u32)) return false;
  if (u32 != 0x46464952) return false; // "RIFF"
  // Skip ChunkSize
  if (!ReadU32(&u32)) return false; 
  // Format == "WAVE"
  if (!ReadU32(&u32)) return false;
  if (u32 != 0x45564157) return false; // "WAVE"

  // fmt subchunk has the info on the data format
  // id == "fmt "
  if (!ReadU32(&u32)) return false;
  if (u32 != 0x20746d66) return false; // "fmt "
  // subchunk size
  if (!ReadU32(&u32)) return false;
  if (u32 != 16) return false; // we only do standard PCM
  // AudioFormat
  if (!ReadU16(&u16)) return false;
  if (u16 != 1) return false; // we only do standard PCM
  // NumChannels
  if (!ReadU16(&channels)) return false;
  if ((channels<1) || (channels>2)) return false; // Mono or stereo support only
  // SampleRate
  if (!ReadU32(&sampleRate)) return false;
  if (sampleRate < 1) return false; // Weird rate, punt.  Will need to check w/DAC to see if supported
  // Ignore byterate and blockalign
  if (!ReadU32(&u32)) return false;
  if (!ReadU16(&u16)) return false;
  // Bits per sample
  if (!ReadU16(&bitsPerSample)) return false;
  if ((bitsPerSample!=8) && (bitsPerSample != 16)) return false; // Only 8 or 16 bits

  // look for data subchunk
  do {
    // id == "data"
    if (!ReadU32(&u32)) return false;
    if (u32 == 0x61746164) break; // "data"
    // Skip size, read until end of chunk
    if (!ReadU32(&u32)) return false;
    file->seek(u32, SEEK_CUR);
  } while (1);
  if (!file->isOpen()) return false;

  // Skip size, read until end of file...
  if (!ReadU32(&u32)) return false;
      
  // Now set up the buffer or fail
  buff = reinterpret_cast<uint8_t *>(malloc(buffSize));
  if (!buff) return false;
  buffPtr = 0;
  buffLen = 0;

  return true;
}

bool AudioGeneratorWAV::begin(AudioFileSource *source, AudioOutput *output)
{
  if (!source) return false;
  file = source;
  if (!output) return false;
  this->output = output;
  if (!file->isOpen()) return false; // Error
  
  if (!ReadWAVInfo()) return false;

  if (!output->SetRate( sampleRate )) return false;
  if (!output->SetBitsPerSample( bitsPerSample )) return false;
  if (!output->SetChannels( channels )) return false;
  if (!output->begin()) return false;

  running = true;
  
  return true;
}

