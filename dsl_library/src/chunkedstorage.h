#ifndef CHUNKEDSTORAGE_HH
#define CHUNKEDSTORAGE_HH

#include <vector>

#include "idxfile.h"

using std::vector;


namespace ChunkedStorage {

class Writer {
  
  IdxFile& file;
  
  vector<uint32_t> offsets;
  vector<unsigned char> buffer;
  vector<unsigned char> bufferCompressed;
  
  bool chunkStarted;
  size_t scratchPadOffset, scratchPadSize;
  size_t bufferUsed;

private:
  void saveCurrentChunk();

public:

  Writer(IdxFile& f);

  uint32_t startNewBlock();
  
  void addToBlock(const void* data, size_t size);
  template<typename T> void addToBlock(const T& value) {
    addToBlock(&value, sizeof(value));
  }
  
  uint32_t finish();
  
};


class Reader {
  
  IdxFile& file;
  vector<uint32_t> offsets;

public:
  Reader(IdxFile &, uint32_t);
  char* getBlock(uint32_t address, vector<char>& chunk);
  
};

}

#endif
