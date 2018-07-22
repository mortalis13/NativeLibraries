#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "vars.h"
#include "log_std.h"
// #include "log_ndk.h"

using namespace std;


class Config {
public:
  static bool TEST_MODE;
  static bool STOP_INDEXING;
  static bool STOP_SEARCH;
};


namespace Utils {

// ---------------------------------------- Dump Bytes ----------------------------------------
void dump_bytes_chars(const uc_string& us);
void dump_bytes(const uc_string& str);
void dump_bytes(const string& str);
void dump_bytes(const char* strPtr, size_t size);


// ---------------------------------------- Conversions ----------------------------------------
uc_char combineChars(char b1, char b2, bool& result);
  
// string to uc_string
bool convert(uc_string& res, const char* fromData, size_t dataSize, Encoding toEnc, Encoding fromEnc);
// uc_string to string
bool convert(string& res, const uc_string& fromStr, Encoding toEnc = ENC_UTF8, Encoding fromEnc = ENC_UCS4LE);

string convert(const uc_string& res);
uc_string convert(const string& str);

uc_string convertBytes(const string& in);
uc_string convertBytes(const char* in, size_t inSize);
long convertBytes(const char* in, size_t inSize, uc_char* out);

string convertBytes(const uc_string& in);
string convertBytes(const uc_char* in, size_t inSize);
size_t convertBytes(const uc_char* in, size_t inSize, char* out);

const char* encToStr(Encoding enc);


// ---------------------------------------- Text Utils ----------------------------------------
bool isWhitespace(uc_char ch);
bool isEmpty(const uc_string& s);
bool isEmpty(const string& s);
bool match(const uc_string& s, const uc_string& sub);
void normalize(string& s);
size_t findNonSpaceChar(const uc_string& s, size_t pos);

bool regexMatch(string text, string pattern);
string getFileName(string filePath);
string getIndexPath(string dictPath, string indexDir);

void getLangInfo(const uc_string& langStr, uint32_t& langCode, string& langCodeStr);

void lineTrim(string& str);
void lineTrim(uc_string& str);
void trim(string& str);
void trim(uc_string& str);
void trimRight(uc_string& str);
string trimNewline(const string& str);

void lineIndent(string& str);

bool isXchar(uc_char c);
void fixUnicode(uc_string& str);

bool cmpWords(const string &s1, const string &s2);

template<typename T> string toStr(T number) {
  ostringstream ss;
  ss << number;
  return ss.str();
}


// ---------------------------------------- Utils ----------------------------------------
bool find(const vector<string>& container, const string& val);


// ---------------------------------------- File IO ----------------------------------------
void writeToFile(fstream& file, const uc_string& str);
void writeToFile(fstream& file, const string& str);
bool fileExists(string fileName);

}

#endif
