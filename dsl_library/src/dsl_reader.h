#ifndef DSL_READER_H
#define DSL_READER_H

#include <string>
#include <vector>
#include <zlib.h>

#include "vars.h"


using std::string;
using std::vector;


class DslReader {
  
  gzFile f;
  
  string dslFileName;
  Encoding sourceEncoding;
  
  uc_string dictionaryName;
  uc_string langFrom;
  uc_string langTo;
  
  unsigned bufIdx;
  unsigned fileOffset;
  unsigned linesRead;
  int bytesRead;
  
  bool fileEnd;
  
  static const unsigned bufSize = 65536;
  char readBuffer[bufSize];

public:

  DslReader(const string& fileName);
  ~DslReader();

  void readHeader();
  void skipEmptyLines();
  bool readLine(uc_string& out, size_t& offset, bool header = false, bool skipEmpty = false);
  bool readLineUtf8(uc_string& out, size_t& offset, bool header, bool skipEmpty);
  
  bool isDslFile();
 
  const uc_string& getDictionaryName() const {
    return dictionaryName;
  }

  const uc_string& getLangFrom() const {
    return langFrom;
  }

  const uc_string& getLangTo() const {
    return langTo;
  }

  unsigned getLinesRead() const {
    return linesRead;
  }

  unsigned getFileOffset() const {
    return fileOffset;
  }

  Encoding getSourceEncoding() const {
    return sourceEncoding;
  }

};

#endif