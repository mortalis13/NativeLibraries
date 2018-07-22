#include "wav_reader.h"

WavReader::WavReader() {
  memset(&wavHeader, 0, sizeof(wavHeader));
}

WavReader::~WavReader() {
  
}

bool WavReader::readWav(string filePath) {
  ifstream file(filePath, ios::binary);

  if (!file.good()) {
    Log::d("ERROR: File doesn't exist or otherwise can't load file: " + filePath);
    return false;
  }

  file.unsetf(ios::skipws);
  istream_iterator<uint8_t> begin(file), end;
  vector<uint8_t> fileData(begin, end);

  // -----------------------------------------------------------
  // HEADER CHUNK
  string headerChunkID(fileData.begin(), fileData.begin() + 4);
  //int32_t fileSizeInBytes = fourBytesToInt (fileData, 4) + 8;
  string format(fileData.begin() + 8, fileData.begin() + 12);

  // -----------------------------------------------------------
  // try and find the start points of key chunks
  int indexOfDataChunk = getIndexOfString(fileData, "data");
  int indexOfFormatChunk = getIndexOfString(fileData, "fmt");

  // if we can't find the data or format chunks, or the IDs/formats don't seem to be as expected
  // then it is unlikely we'll able to read this file, so abort
  if (indexOfDataChunk == -1 || indexOfFormatChunk == -1 || headerChunkID != "RIFF" || format != "WAVE") {
    Log::d("ERROR: this doesn't seem to be a valid .WAV file");
    return false;
  }

  // -----------------------------------------------------------
  // FORMAT CHUNK
  int f = indexOfFormatChunk;
  string formatChunkID(fileData.begin() + f, fileData.begin() + f + 4);
  //int32_t formatChunkSize = fourBytesToInt (fileData, f + 4);
  int16_t audioFormat = twoBytesToInt(fileData, f + 8);
  int16_t numChannels = twoBytesToInt(fileData, f + 10);
  uint32_t sampleRate = (uint32_t) fourBytesToInt(fileData, f + 12);
  int32_t bytesPerSecond = fourBytesToInt(fileData, f + 16);
  int16_t bytesPerBlock = twoBytesToInt(fileData, f + 20);
  int16_t bitsPerSample = twoBytesToInt(fileData, f + 22);

  int bytesPerSample = bitsPerSample / 8;

  wavHeader.numChannels = numChannels;
  wavHeader.sampleRate = sampleRate;
  wavHeader.bytesPerSecond = bytesPerSecond;
  wavHeader.bytesPerBlock = bytesPerBlock;
  wavHeader.bitsPerSample = bitsPerSample;

  // check that the audio format is PCM
  if (audioFormat != 1) {
    Log::d("ERROR: this is a compressed .WAV file and this library does not support decoding them at present");
    return false;
  }

  // check the number of channels is mono or stereo
  if (numChannels < 1 || numChannels > 2) {
    Log::d("ERROR: this WAV file seems to be neither mono nor stereo (perhaps multi-track, or corrupted?)");
    return false;
  }

  // check header data is consistent
  if ((bytesPerSecond != (numChannels * sampleRate * bitsPerSample) / 8) || (bytesPerBlock != (numChannels * bytesPerSample))) {
    Log::d("ERROR: the header data in this WAV file seems to be inconsistent");
    return false;
  }

  // check bit depth is either 8, 16 or 24 bit
  if (bitsPerSample != 8 && bitsPerSample != 16 && bitsPerSample != 24) {
    Log::d("ERROR: this file has a bit depth that is not 8, 16 or 24 bits");
    return false;
  }

  // -----------------------------------------------------------
  // DATA CHUNK
  int d = indexOfDataChunk;
  string dataChunkID(fileData.begin() + d, fileData.begin() + d + 4);
  int32_t dataChunkSize = fourBytesToInt(fileData, d + 4);
  
  int numSamples = dataChunkSize / (numChannels * bitsPerSample / 8);
  int samplesStartIndex = indexOfDataChunk + 8;
  
  auto beginIt = fileData.begin() + indexOfDataChunk + 8;
  auto endIt = beginIt + dataChunkSize;
  audioData = vector<uint8_t>(beginIt, endIt);
  
  
  // clearAudioBuffer();
  if (false) {
    samples.resize(numChannels);
    for (int i = 0; i < numSamples; i++) {
      for (int channel = 0; channel < numChannels; channel++) {
        int sampleIndex = samplesStartIndex + (bytesPerBlock * i) + channel * bytesPerSample;

        if (bitsPerSample == 8) {
          int32_t sampleAsInt = (int32_t) fileData[sampleIndex];
          double sample = (double) (sampleAsInt - 128) / (double) 128.;
          samples[channel].push_back(sample);
        }
        else if (bitsPerSample == 16) {
          int16_t sampleAsInt = twoBytesToInt(fileData, sampleIndex);
          double sample = (double) sampleAsInt / (double) 32768.;
          samples[channel].push_back(sample);
        }
        else if (bitsPerSample == 24) {
          int32_t sampleAsInt = 0;
          sampleAsInt = (fileData[sampleIndex + 2] << 16) | (fileData[sampleIndex + 1] << 8) | fileData[sampleIndex];

          // if the 24th bit is set, this is a negative number in 24-bit world
          // so make sure sign is extended to the 32 bit float
          if (sampleAsInt & 0x800000) sampleAsInt = sampleAsInt | ~0xFFFFFF;
          
          double sample = (double) sampleAsInt / (double) 8388608.;
          samples[channel].push_back(sample);
        }
      }
    } // for
  }
  
  return true;
}


int WavReader::getIndexOfString(vector<uint8_t>& source, string stringToSearchFor) {
  int index = -1;
  int stringLength = (int) stringToSearchFor.length();

  for (int i = 0; i < source.size() - stringLength; i++) {
    string section(source.begin() + i, source.begin() + i + stringLength);

    if (section == stringToSearchFor) {
      index = i;
      break;
    }
  }

  return index;
}

int16_t WavReader::twoBytesToInt(vector<uint8_t>& source, int startIndex) {
  int16_t result;
  result = (source[startIndex + 1] << 8) | source[startIndex];
  return result;
}

int32_t WavReader::fourBytesToInt(vector<uint8_t>& source, int startIndex) {
  int32_t result;
  result = (source[startIndex + 3] << 24) | (source[startIndex + 2] << 16) | (source[startIndex + 1] << 8) | source[startIndex];
  return result;
}
