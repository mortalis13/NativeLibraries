#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "vars.h"
// #include "log_std.h"
#include "log_ndk.h"

using namespace std;


class Config {
public:
  static bool TEST_MODE;
  static bool STOP_INDEXING;
  static bool STOP_SEARCH;
};


namespace Utils {

  void dumpBytes(const vector<uint8_t>& data);
  void dumpBytesAsStr(const vector<uint8_t>& data);
  void dumpBytesAsStr(uint8_t* data, int len);

}

#endif
