
#include "utils.h"

#include <regex>
#include <iconv.h>
#include <stdarg.h>


bool Config::TEST_MODE = false;
bool Config::STOP_INDEXING = false;
bool Config::STOP_SEARCH = false;


namespace Utils {

  void dumpBytes(const vector<uint8_t>& data) {
    int lineSize = 100;
    for (size_t i = 0; i < data.size(); i++) {
      Log::dn("%02x ", data[i]);
      if (i != 0 && i % lineSize == 0) Log::d("");
    }
  }

  void dumpBytesAsStr(const vector<uint8_t>& data) {
    string bytesStr;
    for (size_t i = 0; i < data.size(); i++) {
      char buf[10];
      int n = sprintf(buf, "%02x ", data[i]);
      bytesStr += string(buf, n);
    }
    Log::d(bytesStr);
  }

  void dumpBytesAsStr(uint8_t* data, int len) {
    string bytesStr;
    for (size_t i = 0; i < len; i++) {
      char buf[10];
      int n = sprintf(buf, "%02x ", data[i]);
      bytesStr += string(buf, n);
    }
    Log::d(bytesStr);
  }

}
// namespace
