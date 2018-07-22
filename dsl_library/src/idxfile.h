#ifndef IDXFILE_HH
#define IDXFILE_HH

#include <cstdio>
#include <string>
#include <fstream>


using std::string;
using std::fstream;
using std::ios_base;


class IdxFile {
  
  fstream f;
  const char* filename;

private:
  void open(const char* filename, const char* mode);

public:
  
  IdxFile() {}
  IdxFile(const char* filename, const char* mode);
  ~IdxFile();

  void read(void* buf, int64_t size);

  template<typename T> void read(T& value) {
    read(&value, sizeof(value));
  }

  template<typename T> T read() {
    T value;
    read(value);
    return value;
  }

  void write(const void* buf, int64_t size);

  template<typename T> void write(const T& value) {
    write(&value, sizeof(value));
  }

  void seek(long offset);
  void rewind();
  size_t tell();
  void close();

};

#endif