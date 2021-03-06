/*
  AudioFileSourceHTTPStream
  Streaming HTTP source
  
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

#include "AudioFileSourceHTTPStream.h"

AudioFileSourceHTTPStream::AudioFileSourceHTTPStream()
{
  pos = 0;
}

AudioFileSourceHTTPStream::AudioFileSourceHTTPStream(const char *url)
{
  open(url);
}

bool AudioFileSourceHTTPStream::open(const char *url)
{
  pos = 0;
  http.begin(url);
  http.setReuse(true);
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    http.end();
    return false;
  }
  return true;
}

AudioFileSourceHTTPStream::~AudioFileSourceHTTPStream()
{
  http.end();
}

uint32_t AudioFileSourceHTTPStream::read(void *data, uint32_t len)
{
  if (!http.connected()) return 0;

  WiFiClient *stream = http.getStreamPtr();
  while (stream->available() < (int)len) yield();

  size_t avail = stream->available();
  if (!avail) return 0;
  if (avail < len) len = avail;

  int read = stream->readBytes(reinterpret_cast<uint8_t*>(data), len);
  pos += read;
  return read;
}

bool AudioFileSourceHTTPStream::seek(int32_t pos, int dir)
{
  (void) pos;
  (void) dir;
  return false;
}

bool AudioFileSourceHTTPStream::close()
{
  http.end();
  return true;
}

bool AudioFileSourceHTTPStream::isOpen()
{
  return http.connected();
}

uint32_t AudioFileSourceHTTPStream::getSize()
{
  return 1L<<31;
}

uint32_t AudioFileSourceHTTPStream::getPos()
{
  return pos;
}
