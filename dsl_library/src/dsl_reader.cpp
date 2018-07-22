
#include "dsl_reader.h"

#include <stdio.h>

#include "dsl_utils.h"
#include "utils.h"


DslReader::DslReader(const string& fileName)
: dslFileName(fileName),
  bufIdx(0),
  fileOffset(0),
  linesRead(0),
  bytesRead(-1),
  fileEnd(false)
{
  f = gzopen(fileName.c_str(), "rb");
  if (!f) throw runtime_error(Vars::ERROR_CANNOT_OPEN_DSL_FILE + ": " + dslFileName);
  
  unsigned char buf[2];
  if (gzread(f, buf, sizeof(buf)) != sizeof(buf)) {
    gzclose(f);
    throw runtime_error(Vars::ERROR_INCORRECT_DSL_FILE + ": " + dslFileName);
  }
  
  sourceEncoding = ENC_UTF16LE;
  
  // BOM 0xFFFE -> LE, 0xFEFF -> BE
  if (buf[0] == 0xFF && buf[1] == 0xFE) {
    // UTF-16LE with BOM
    fileOffset += sizeof(buf);
  }
  else if (buf[1] == 0x00) {
    // UTF-16LE
    gzrewind(f);
  }
  else {
    // UTF-8
    Log::dg("Detected UTF-8 encoding");
    sourceEncoding = ENC_UTF8;
    gzrewind(f);
    
    unsigned char addBuf[3];
    if (gzread(f, addBuf, sizeof(addBuf)) != sizeof(addBuf)) {
      gzclose(f);
      throw runtime_error(Vars::ERROR_INCORRECT_DSL_FILE + ": " + dslFileName);
    }
    
    // UTF-8 with BOM
    if (addBuf[0] == 0xEF && addBuf[1] == 0xBB && addBuf[2] == 0xBF) {
      fileOffset += sizeof(addBuf);
    }
    else {
      gzrewind(f);
    }
    
    // throw runtime_error(Vars::ERROR_DSL_ENCODING + ": " + dslFileName);
  }
  
  // read first portion
  bytesRead = gzread(f, readBuffer, bufSize);
  if (bytesRead == -1) throw runtime_error(Vars::ERROR_READ_DSL_FILE + ": " + dslFileName);
}

DslReader::~DslReader() {
  gzclose(f);
}


void DslReader::readHeader() {
  Log::d("readHeader()");
  
  uc_string line;
  size_t offset;
  
  while (true) {
    bool res = readLine(line, offset, true);
    if (!res) break;
    
    size_t beg = line.find_first_of(U'"');
    size_t end = line.find_last_of(U'"');
    
    if (beg == npos || end == npos) {
      Log::e("No \" found in the header line");
      throw runtime_error(Vars::ERROR_INCORRECT_DSL_FILE + ": " + dslFileName);
    }
    
    uc_string headerVal(line, beg + 1, end - beg - 1);
    if (line.find(U"#NAME", 0) != npos) {
      dictionaryName = headerVal;
    }
    else if (line.find(U"#INDEX_LANGUAGE", 0) != npos) {
      langFrom = headerVal;
    }
    else if (line.find(U"#CONTENTS_LANGUAGE", 0) != npos) {
      langTo = headerVal;
    }
  }
}


void DslReader::skipEmptyLines() {
  uc_string str;
  size_t offset;
  while (true) {
    if (!readLine(str, offset, false, true)) break;
  }
}


bool DslReader::readLine(uc_string& out, size_t& offset, bool header, bool skipEmpty) {
  if (sourceEncoding == ENC_UTF8) return readLineUtf8(out, offset, header, skipEmpty);
  
  out.erase();
  
  uc_string str;
  bool hasComment = false;
  
  do {
    offset = fileOffset;
    if (fileEnd) return false;

    vector<char> buf;
    
    while (true) {
      if (bufIdx == (unsigned) bytesRead) {
        if (gzeof(f)) {
          fileEnd = true;
          break;
        }
        
        bytesRead = gzread(f, readBuffer, bufSize);
        // Log::d("gzread: %d", bytesRead);
        if (bytesRead == -1) throw runtime_error(Vars::ERROR_READ_DSL_FILE + ": " + dslFileName);
        bufIdx = 0;
      }
      
      char b1 = readBuffer[bufIdx];
      char b2 = readBuffer[bufIdx + 1];
      
      // header ends with an empty line or when a line doesn't start with #
      if (header && buf.size() == 0 && !DslUtils::isHeaderStart(b1, b2)) return false;
      // after header it skips empty lines or non-headword lines
      if (skipEmpty && buf.size() == 0 && DslUtils::isHeadwordStart(b1, b2)) return false;
      
      bufIdx += 2;
      fileOffset += 2;
      
      if (b1 == 0x0d && b2 == 0x00) continue;   // \r
      if (b1 == 0x0a && b2 == 0x00) break;      // \n
      
      buf.push_back(b1);
      buf.push_back(b2);
    }
    
    linesRead++;
    
    bool res = Utils::convert(str, &buf.front(), buf.size(), ENC_UCS4LE, ENC_UTF16LE);
    if (!res) Log::e("Error converting text at line: %d", linesRead);
    
    DslUtils::stripComments(str, hasComment);
    out += str;
  }
  while (hasComment);
  
  // Log::d(out);
  
  return true;
}


bool DslReader::readLineUtf8(uc_string& out, size_t& offset, bool header, bool skipEmpty) {
  out.erase();
  
  uc_string str;
  bool hasComment = false;
  
  do {
    offset = fileOffset;
    if (fileEnd) return false;

    vector<char> buf;
    
    while (true) {
      if (bufIdx == (unsigned) bytesRead) {
        if (gzeof(f)) {
          fileEnd = true;
          break;
        }
        
        bytesRead = gzread(f, readBuffer, bufSize);
        // Log::d("gzread: %d", bytesRead);
        if (bytesRead == -1) throw runtime_error(Vars::ERROR_READ_DSL_FILE + ": " + dslFileName);
        bufIdx = 0;
      }
      
      char b1 = readBuffer[bufIdx];
      
      // header ends with an empty line or when a line doesn't start with #
      if (header && buf.size() == 0 && !DslUtils::isHeaderStart(b1)) return false;
      // after header it skips empty lines or non-headword lines
      if (skipEmpty && buf.size() == 0 && DslUtils::isHeadwordStart(b1)) return false;
      
      bufIdx++;
      fileOffset++;
      
      if (b1 == 0x0d) continue;   // \r
      if (b1 == 0x0a) break;      // \n
      
      buf.push_back(b1);
    }
    
    linesRead++;
    
    bool res = Utils::convert(str, &buf.front(), buf.size(), ENC_UCS4LE, ENC_UTF8);
    if (!res) Log::e("--Error converting text at line: %d", linesRead);
    
    DslUtils::stripComments(str, hasComment);
    out += str;
  }
  while (hasComment);
  
  // Log::d(out);
  
  return true;
}

bool DslReader::isDslFile() {
  return !dictionaryName.empty() && !langFrom.empty() && !langTo.empty();
}
