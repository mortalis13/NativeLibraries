#ifndef WAV_READER_H
#define WAV_READER_H

#include <algorithm>
#include <string>
#include <cstring>
#include <vector>
#include <fstream>
#include <iterator>
#include <stdlib.h>

#include "utils.h"

using namespace std;


struct WavHeader {
  int16_t numChannels;
  uint32_t sampleRate;
  int32_t bytesPerSecond;
  int16_t bytesPerBlock;
  int16_t bitsPerSample;
};


class WavReader {

public:
  
  WavReader();
  ~WavReader();
  
  bool readWav(string filePath);

  int16_t getNumChannels() {
    return wavHeader.numChannels;
  }
  uint32_t getSampleRate() {
    return wavHeader.sampleRate;
  }
  int32_t getBytesPerSecond() {
    return wavHeader.bytesPerSecond;
  }
  int16_t getBytesPerBlock() {
    return wavHeader.bytesPerBlock;
  }
  int16_t getBitsPerSample() {
    return wavHeader.bitsPerSample;
  }

  vector<uint8_t> getAudioData() {
    return audioData;
  }


private:

  int getIndexOfString(vector<uint8_t>& source, string stringToSearchFor);
  int16_t twoBytesToInt(vector<uint8_t>& source, int startIndex);
  int32_t fourBytesToInt(vector<uint8_t>& source, int startIndex);


private:

  WavHeader wavHeader;
  vector<vector<double>> samples;
  vector<uint8_t> audioData;

};

#endif
