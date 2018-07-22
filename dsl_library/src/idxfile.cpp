#include "idxfile.h"

#include <cstring>
#include <cerrno>
#include <string>

#include "vars.h"


IdxFile::IdxFile(const char* filename, const char* mode) {
  this->filename = filename;
  open(filename, mode);
}

IdxFile::~IdxFile() {
  f.close();
}

void IdxFile::open(const char* filename, const char* mode) {
  ios_base::openmode flags;
    
  if (strcmp(mode, "r") == 0) flags = ios_base::in;
  else if (strcmp(mode, "r+") == 0) flags = ios_base::in | ios_base::out;
  else if (strcmp(mode, "rb") == 0) flags = ios_base::in | ios_base::binary;
  else if (strcmp(mode, "rb+") == 0) flags = ios_base::in | ios_base::out | ios_base::binary;
  
  else if (strcmp(mode, "w") == 0) flags = ios_base::out;
  else if (strcmp(mode, "w+") == 0) flags = ios_base::out | ios_base::in | ios_base::trunc;
  else if (strcmp(mode, "wb") == 0) flags = ios_base::out | ios_base::binary;
  else if (strcmp(mode, "wb+") == 0) flags = ios_base::out | ios_base::in | ios_base::trunc | ios_base::binary;
  
  else if (strcmp(mode, "a") == 0) flags = ios_base::app;
  else if (strcmp(mode, "a+") == 0) flags = ios_base::app | ios_base::in;
  else if (strcmp(mode, "ab") == 0) flags = ios_base::app | ios_base::binary;
  else if (strcmp(mode, "ab+") == 0) flags = ios_base::app | ios_base::in | ios_base::binary;
  
  f.open(filename, flags);
  
  if (!f.is_open()) {
    throw runtime_error(Vars::ERROR_CANNOT_OPEN_FILE + ": " + string(filename));
  }
}

void IdxFile::read(void* buf, int64_t size) {
  if (!size) return;
  f.read((char*) buf, size);
  
  if (f.gcount() != size) {
    throw runtime_error(Vars::ERROR_READ_FILE + ": " + string(filename));
  }
}

void IdxFile::write(const void* buf, int64_t size) {
  if (!size) return;

  f.write((char*) buf, size);
}

void IdxFile::seek(long offset) {
  f.seekg(offset);
}

void IdxFile::rewind() {
  seek(0);
}

size_t IdxFile::tell() {
  return f.tellg();
}

void IdxFile::close() {
  f.close();
}
