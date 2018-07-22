#include "chunkedstorage.h"

#include <zlib.h>
#include <string.h>

#include "vars.h"
#include "utils.h"


namespace ChunkedStorage {

// Can't be more since it would overflow the address
#define CHUNK_MAX_SIZE 65536


Writer::Writer(IdxFile& f):
  file(f), chunkStarted(false), bufferUsed(0)
{
  char zero[4096];
  memset(zero, 0, sizeof(zero));

  scratchPadOffset = file.tell();
  scratchPadSize = sizeof(zero);

  file.write(zero, sizeof(zero));
}

uint32_t Writer::startNewBlock() {
  if (bufferUsed >= CHUNK_MAX_SIZE) {
    saveCurrentChunk();
  }

  chunkStarted = true;
  return bufferUsed | ((uint32_t) offsets.size() << 16);
}

void Writer::addToBlock(const void* data, size_t size) {
  if (!size) return;

  if (buffer.size() - bufferUsed < size)
    buffer.resize(bufferUsed + size);

  memcpy(&buffer.front() + bufferUsed, data, size);
  bufferUsed += size;
  chunkStarted = false;
}

void Writer::saveCurrentChunk() {
  size_t maxCompressedSize = compressBound(bufferUsed);

  if (bufferCompressed.size() < maxCompressedSize) {
    bufferCompressed.resize(maxCompressedSize);
  }

  unsigned long compressedSize = bufferCompressed.size();
  bool res = compress(&bufferCompressed.front(), &compressedSize, &buffer.front(), bufferUsed);
  if (res != Z_OK) {
    throw runtime_error(Vars::ERROR_COMPRESS_CHUNK);
  }

  offsets.push_back(file.tell());

  file.write((uint32_t) bufferUsed);
  file.write((uint32_t) compressedSize);
  file.write(&bufferCompressed.front(), compressedSize);

  bufferUsed = 0;
  chunkStarted = false;
}

uint32_t Writer::finish() {
  if (bufferUsed || chunkStarted) {
    saveCurrentChunk();
  }

  bool useScratchPad = false;
  uint32_t savedOffset = 0;

  if (scratchPadSize >= offsets.size() * sizeof(uint32_t) + sizeof(uint32_t)) {
    useScratchPad = true;
    savedOffset = file.tell();
    file.seek(scratchPadOffset);
  }

  uint32_t offset = file.tell();
  file.write((uint32_t) offsets.size());

  if (offsets.size()) {
    file.write(&offsets.front(), offsets.size() * sizeof(uint32_t));
  }
  
  if (useScratchPad) file.seek(savedOffset);

  offsets.clear();
  chunkStarted = false;

  return offset;
}


Reader::Reader(IdxFile& f, uint32_t offset): file(f) {
  file.seek(offset);

  uint32_t size =  file.read<uint32_t>();
  if (size == 0)
    return;
  
  offsets.resize(size);
  file.read(&offsets.front(), offsets.size() * sizeof(uint32_t));
}

char* Reader::getBlock(uint32_t address, vector<char>& chunk) {
  size_t chunkIdx = address >> 16;

  if (chunkIdx >= offsets.size()) {
    throw runtime_error(Vars::ERROR_CHUNK_ADDRESS_OUT_OF_RANGE);
  }

  // Read and decompress the chunk
  file.seek(offsets[ chunkIdx ]);

  uint32_t uncompressedSize = file.read<uint32_t>();
  uint32_t compressedSize = file.read<uint32_t>();

  chunk.resize(uncompressedSize);
  vector<unsigned char> compressedData(compressedSize);
  file.read(&compressedData.front(), compressedData.size());

  unsigned long decompressedLength = chunk.size();

  bool res = uncompress((unsigned char *)&chunk.front(), &decompressedLength, &compressedData.front(), compressedData.size());
  if (res != Z_OK || decompressedLength != chunk.size()) {
    throw runtime_error(Vars::ERROR_DECOMPRESS_CHUNK);
  }

  size_t offsetInChunk = address & 0xffFF;

  // It can be equal to for 0-sized blocks
  if (offsetInChunk > chunk.size()) {
    throw runtime_error(Vars::ERROR_CHUNK_ADDRESS_OUT_OF_RANGE);
  }

  return &chunk.front() + offsetInChunk;
}

}
