
#include "utils.h"

#include <regex>
#include <iconv.h>
#include <stdarg.h>

#include "folding.h"
#include "xchars.h"


bool Config::TEST_MODE = false;
bool Config::STOP_INDEXING = false;
bool Config::STOP_SEARCH = false;


namespace Utils {

// ---------------------------------------- Dump Bytes ----------------------------------------

void dump_bytes_chars(const uc_string& us) {
  int line_width = 16;
  // int line_width = 24;
  int byte_pad = 4;
  
  register int i = 0, j;
  uc_char c[line_width];
  
  int totalRead = 0;
  int len = us.length();
  
  cout << "\n-- text dump --\n";
  for (uc_char uc: us) {
    cout << setfill(' ') << setw(byte_pad) << hex << uc;
    totalRead++;
    
    c[i++] = uc;
    if (i == line_width || totalRead == len) {
      for (j = i*(byte_pad+1); j<line_width*(byte_pad+1); j++)
        cout << " ";
      cout << "\t|";
      
      for (j = 0; j<i; j++) {
        char ch = (char) c[j];
        if (isprint(ch)) cout << ch;
        else cout << '.';
      }
      cout << '\n';
      i = 0;
    }
    else {
      cout << " ";
    }
  }
  cout << "\n-- text dump --\n";
}

void dump_bytes(const uc_string& str) {
  string bytesStr;
  
  char* pStr = (char*) str.data();
  size_t len = str.size() * sizeof(uc_char);
  
  for (size_t i = 0; i < len; i++) {
    char buf[10];
    int n = sprintf(buf, "%02x ", pStr[i]);
    bytesStr += string(buf, n);
  }
  
  Log::d(bytesStr);
}

void dump_bytes(const string& str) {
  string sep = "-";
  string bytesStr;
  
  for (auto c: str) {
    char buf[10];
    int n = sprintf(buf, "%02x ", (unsigned char) c);
    bytesStr += string(buf, n);
  }
  
  Log::d(bytesStr);
}

void dump_bytes(const char* strPtr, size_t size) {
  string bytesStr;
  for (size_t i = 0; i < size; i++) {
    char buf[10];
    int n = sprintf(buf, "%02x ", (unsigned char) strPtr[i]);
    bytesStr += string(buf, n);
  }
  Log::d(bytesStr);
}


// ---------------------------------------- Conversions ----------------------------------------

uc_char combineChars(char b1, char b2, bool& result) {
  char src[] = {b1, b2};
  uc_char outBuf[1];
  
  char *pIn = src;
  uc_char *pOut = outBuf;
  
  size_t inRemains = 2;
  size_t outRemains = 2;

  iconv_t cb = iconv_open("UCS-2LE", "UTF-16LE");
  size_t cvtlen = iconv(cb, (char**) &pIn, &inRemains, (char**) &pOut, &outRemains);
  iconv_close(cb);
  
  if (cvtlen == (size_t) -1) {
    Log::eg("iconv error (combineChars()): [%d] %s, 0x%02x, 0x%02x\n", errno, strerror(errno), (unsigned char) b1, (unsigned char) b2);
    Log::eg("inRemains: %d, outRemains: %d, cvtlen: %d\n", inRemains, outRemains, cvtlen);
    result = false;
  }
  
  uc_char c16 = outBuf[0];
  return c16;
}

// string to uc_string
bool convert(uc_string& res, const char* fromData, size_t dataSize, Encoding toEnc, Encoding fromEnc) {
  size_t strSize = dataSize/2;
  if (fromEnc == ENC_UTF8) {
    strSize = dataSize;
  }
  
  size_t outSize = strSize * sizeof(uc_char);
  // uc_char outBuf[strSize];
  uc_char* outBuf = new uc_char[strSize];
  
  const char *pIn = fromData;
  uc_char *pOut = outBuf;
  
  size_t inRemains = dataSize;
  size_t outRemains = outSize;
  
  // Log::d("inRemains: %d, outRemains: %d\n", inRemains, outRemains);
  
  const char* toEncStr = encToStr(toEnc);
  const char* fromEncStr = encToStr(fromEnc);
  
  iconv_t cb = iconv_open(toEncStr, fromEncStr);
  size_t cvtlen = iconv(cb, (char**) &pIn, &inRemains, (char**) &pOut, &outRemains);
  iconv_close(cb);
  
  // Log::d("inRemains: %d, outRemains: %d, cvtlen: %d\n", inRemains, outRemains, cvtlen);
  
  size_t resSize = (outSize - outRemains) / sizeof(uc_char);
  res = uc_string(outBuf, resSize);
  delete[] outBuf;
  
  if (cvtlen == (size_t) -1) {
    Log::eg("iconv error (convert(char*, size_t)): [%d] %s, %s\n", errno, strerror(errno), string(fromData, dataSize).c_str());
    dump_bytes(fromData, dataSize);
    Log::eg("dataSize: %d\n", dataSize);
    Log::eg("inRemains: %d, outRemains: %d, cvtlen: %d\n", inRemains, outRemains, cvtlen);
    return false;
  }
  
  return true;
}


// uc_string to string
bool convert(string& res, const uc_string& fromStr, Encoding toEnc, Encoding fromEnc) {
  size_t strSize = fromStr.size() * sizeof(uc_char);
  size_t outSize = strSize;
  
  // char outBuf[outSize];
  char* outBuf = new char[outSize];
  
  const uc_char *pIn = fromStr.data();
  char *pOut = outBuf;
  
  size_t inRemains = strSize;
  size_t outRemains = outSize;
  
  // Log::d("inRemains: %d, outRemains: %d\n", inRemains, outRemains);
  
  const char* toEncStr = encToStr(toEnc);
  const char* fromEncStr = encToStr(fromEnc);
  
  iconv_t cb = iconv_open(toEncStr, fromEncStr);
  size_t cvtlen = iconv(cb, (char**) &pIn, &inRemains, (char**) &pOut, &outRemains);
  iconv_close(cb);
  
  // Log::d("inRemains: %d, outRemains: %d, cvtlen: %d\n", inRemains, outRemains, cvtlen);
  
  size_t resSize = outSize - outRemains;
  res = string(outBuf, resSize);
  delete[] outBuf;
  
  if (cvtlen == (size_t) -1) {
    Log::eg("iconv error (convert(uc_string)): [%d] %s\n", errno, strerror(errno));
    Log::eg("inRemains: %d, outRemains: %d, cvtlen: %d\n", inRemains, outRemains, cvtlen);
    dump_bytes(fromStr);
    return false;
  }
  
  return true;
}

string convert(const uc_string& str) {
  string result;
  convert(result, str);
  return result;
}

uc_string convert(const string& str) {
  size_t dataSize = str.size();
  uc_char outBuf[dataSize];
  
  const char *pIn = str.data();
  uc_char *pOut = outBuf;
  
  size_t inRemains = dataSize;
  size_t outRemains = dataSize * sizeof(uc_char);

  // Log::d("inRemains: %d, outRemains: %d\n", inRemains, outRemains);
  
  iconv_t cb = iconv_open(Vars::DATA_ENCODING, Vars::STR_ENCODING);
  size_t cvtlen = iconv(cb, (char**) &pIn, &inRemains, (char**) &pOut, &outRemains);
  iconv_close(cb);
  
  // Log::d("inRemains: %d, outRemains: %d, cvtlen: %d\n", inRemains, outRemains, cvtlen);
  
  if (cvtlen == (size_t) -1) {
    Log::eg("iconv error (convert(string)): [%d] %s, %s\n", errno, strerror(errno), str.c_str());
    Log::eg("inRemains: %d, outRemains: %d, cvtlen: %d\n", inRemains, outRemains, cvtlen);
  }
  
  uc_string res(outBuf, dataSize - outRemains / sizeof(*outBuf));
  return res;
}


string convertBytes(const uc_string& in) {
  vector<char> buffer(in.size() * sizeof(uc_char));
  return string(&buffer.front(), convertBytes(in.data(), in.size(), &buffer.front()));
}

string convertBytes(const uc_char* in, size_t inSize) {
  vector<char> buffer(inSize * sizeof(uc_char));
  return string(&buffer.front(), convertBytes(in, inSize, &buffer.front()));
}

size_t convertBytes(const uc_char* in, size_t inSize, char* out_) {
  unsigned char * out = (unsigned char *) out_;

  while(inSize--) {
    if (*in < 0x80) {
      *out++ = *in++;
    }
    else if (*in < 0x800) {
      *out++ = 0xC0 | (*in >> 6);
      *out++ = 0x80 | (*in++ & 0x3F);
    }
    else if (*in < 0x10000) {
      *out++ = 0xE0 | (*in >> 12);
      *out++ = 0x80 | ((*in >> 6) & 0x3F);
      *out++ = 0x80 | (*in++ & 0x3F);
    }
    else {
      *out++ = 0xF0 | (*in >> 18);
      *out++ = 0x80 | ((*in >> 12) & 0x3F);
      *out++ = 0x80 | ((*in >> 6) & 0x3F);
      *out++ = 0x80 | (*in++ & 0x3F);
    }
  }

  return out - (unsigned char *) out_;
}


uc_string convertBytes(const string& in) {
  if (in.size() == 0) return uc_string();

  vector<uc_char> buffer(in.size());
  long result = convertBytes(in.data(), in.size(), &buffer.front());

  if (result < 0) throw runtime_error(Vars::ERROR_CONVERT_STRING + ": " + in);
  return uc_string(&buffer.front(), result);
}

uc_string convertBytes(const char* in, size_t inSize) {
  if (inSize == 0) return uc_string();

  vector<uc_char> buffer(inSize);
  long result = convertBytes(in, inSize, &buffer.front());

  if (result < 0) throw runtime_error(Vars::ERROR_CONVERT_STRING + ": " + in);
  return uc_string(&buffer.front(), result);
}

long convertBytes(const char* in_, size_t inSize, uc_char* out_) {
  unsigned char const * in = (unsigned char const *) in_;
  uc_char * out = out_;

  while(inSize--) {
    uc_char result;

    if (*in & 0x80) {
      if (*in & 0x40) {
        if (*in & 0x20) {
          if (*in & 0x10) {
            // Four-byte sequence
            if (*in & 8)
              // This can't be
              return -1;

            if (inSize < 3)
              return -1;

            inSize -= 3;

            result = ((uc_char)*in++ & 7) << 18;

            if ((*in & 0xC0) != 0x80)
              return -1;
            result |= ((uc_char)*in++ & 0x3F) << 12;

            if ((*in & 0xC0) != 0x80)
              return -1;
            result |= ((uc_char)*in++ & 0x3F) << 6;

            if ((*in & 0xC0) != 0x80)
              return -1;
            result |= (uc_char)*in++ & 0x3F;
          }
          else {
            // Three-byte sequence
            if (inSize < 2)
              return -1;

            inSize -= 2;

            result = ((uc_char)*in++ & 0xF) << 12;

            if ((*in & 0xC0) != 0x80)
              return -1;
            result |= ((uc_char)*in++ & 0x3F) << 6;

            if ((*in & 0xC0) != 0x80)
              return -1;
            result |= (uc_char)*in++ & 0x3F;
          }
        }
        else {
          // Two-byte sequence
          if (!inSize)
            return -1;

          --inSize;
          result = ((uc_char)*in++ & 0x1F) << 6;
          if ((*in & 0xC0) != 0x80) return -1;
          
          result |= (uc_char)*in++ & 0x3F;
        }
      }
      else {
        // This char is from the middle of encoding, it can't be leading
        return -1;
      }
    }
    else {
      // One-byte encoding
      result = *in++;
    }

    *out++ = result;
  }

  return out - out_;
}

const char* encToStr(Encoding enc) {
  switch (enc) {
    case ENC_UTF16LE: return "UTF-16LE";
    case ENC_UTF8: return "UTF-8";
    case ENC_UCS4LE: return "UCS-4LE";
  }
  
  return "NO_ENC";
}


// ---------------------------------------- Text Utils ----------------------------------------

template<typename T> string regexCode(T code) {
  switch (code) {
    case regex_constants::error_collate:
      return "error_collate: "
             "regex has invalid collating element name";
    case regex_constants::error_ctype:
      return "error_ctype: "
             "regex has invalid character class name";
    case regex_constants::error_escape:
      return "error_escape: "
             "regex has invalid escaped char. or trailing escape";
    case regex_constants::error_backref:
      return "error_backref: "
             "regex has invalid back reference";
    case regex_constants::error_brack:
      return "error_brack: "
             "regex has mismatched '[' and ']'";
    case regex_constants::error_paren:
      return "error_paren: "
             "regex has mismatched '(' and ')'";
    case regex_constants::error_brace:
      return "error_brace: "
             "regex has mismatched '{' and '}'";
    case regex_constants::error_badbrace:
      return "error_badbrace: "
             "regex has invalid range in {} expression";
    case regex_constants::error_range:
      return "error_range: "
             "regex has invalid character range, such as '[b-a]'";
    case regex_constants::error_space:
      return "error_space: "
             "insufficient memory to convert regex into finite state";
    case regex_constants::error_badrepeat:
      return "error_badrepeat: "
             "one of *?+{ not preceded by valid regex";
    case regex_constants::error_complexity:
      return "error_complexity: "
             "complexity of match against regex over pre-set level";
    case regex_constants::error_stack:
      return "error_stack: "
             "insufficient memory to determine regex match";
  }
  return "unknown/non-standard regex error code";
}

int compareUcStrings(const uc_char *s1, const uc_char *s2) {
  while(true) {
    if (towlower(*s1) != towlower(*s2)) {
       return towlower(*s1) > towlower(*s2) ? 1 : -1;
    }
    if (!*s1) break;
    
    s1++;
    s2++;
  }
  return 0;
}

bool isWhitespace(uc_char ch) {
  switch (ch) {
    case ' ':
    case '\t':
      return true;
    default:
      return false;
  }
}

bool isEmpty(const uc_string& s) {
  for (auto c: s) {
    if (c != U' ' && c != U'\t' && c != U'\r' && c != U'\n') return false;
  }
  return true;
}

bool isEmpty(const string& s) {
  for (auto c: s) {
    if (c != ' ' && c != '\t' && c != '\r' && c != '\n') return false;
  }
  return true;
}

bool match(const uc_string& s, const uc_string& sub) {
  return s.find(sub) != npos;
}

void normalize(string& s) {
  string p1 = R"(^[ \t]+)";
  string p2 = R"(\n[ \t]+)";
  string p3 = R"((\r?\n)+)";
  
  try {
    s = regex_replace(s, regex(p1), "");
    s = regex_replace(s, regex(p2), "\n");
    s = regex_replace(s, regex(p3), "\n");
  }
  catch (const regex_error& e) {
    string code_str = regexCode(e.code());
    Log::d("Regex error: [%s], %s, code: %s\n", p1.c_str(), e.what(), code_str.c_str());
  }
}

size_t findNonSpaceChar(const uc_string& s, size_t pos) {
  auto len = s.size();
  for (size_t i = pos; i < len; i++) {
    if (s[i] != U' ') return i;
  }
  
  return (size_t) -1;
}

bool regexMatch(string text, string pattern) {
  regex rx(pattern);
  return regex_search(text, rx);
}

string getFileName(string filePath) {
  string res = filePath;
  string p = R"(([^/\\]+)\.[^.]+$)";
  
  try {
    regex rx(p);
    
    smatch m;
    bool found = regex_search(filePath, m, rx);
    
    if (found) {
      res = m.str(1);
    }
  }
  catch (const regex_error& e) {
    string code_str = regexCode(e.code());
    Log::e("Regex error: [%s], %s, code: %s\n", p.c_str(), e.what(), code_str.c_str());
  }
  
  return res;
}

string getIndexPath(string dictPath, string indexDir) {
  string dictFileName = getFileName(dictPath);
  string indexPath = indexDir + dictFileName + ".idx";
  return indexPath;
}

void getLangInfo(const uc_string& langStr, uint32_t& langCode, string& langCodeStr) {
  if (Vars::langMap.find(langStr) != Vars::langMap.end()) {
    Vars::DslLang lang = Vars::langMap[langStr];
    langCode = lang.code;
    langCodeStr = lang.codeStr;
  }
  else {
    langCode = 0;
    langCodeStr = "";
  }
}

void lineTrim(string& str) {
  string res, lineStr;
  
  pos_t pos = 0;
  pos_t line_end_pos = 0;
  
  size_t len = str.size();
  while (pos < len) {
    line_end_pos = str.find_first_of("\r\n", pos);
    lineStr = str.substr(pos, line_end_pos - pos);
    
    // for (pos_t linePos = 0; linePos < lineStr.size(); linePos++) {
    //   if (lineStr[linePos] != '\t' && lineStr[linePos] != ' ') {
    //     lineStr = lineStr.substr(linePos);
    //     break;
    //   }
    // }
    
    trim(lineStr);
    if (!lineStr.empty())
      res += lineStr + "\n";
    
    pos = line_end_pos;
    if (str[pos] == '\r') pos++;
    if (str[pos] == '\n') pos++;
  }
  
  if (!res.empty()) str = res;
}

void lineTrim(uc_string& str) {
  uc_string res, lineStr;
  
  pos_t pos = 0;
  pos_t line_end_pos = 0;
  
  size_t len = str.size();
  while (pos < len) {
    line_end_pos = str.find_first_of(U"\r\n", pos);
    lineStr = str.substr(pos, line_end_pos - pos);
    
    trim(lineStr);
    if (!lineStr.empty())
      res += lineStr + U"\n";
    
    pos = line_end_pos;
    if (str[pos] == U'\r') pos++;
    if (str[pos] == U'\n') pos++;
  }
  
  if (!res.empty()) str = res;
}

void trim(string& str) {
  string res;
  
  size_t pos_start = str.find_first_not_of(" \t\r\n");
  size_t pos_end = str.find_last_not_of(" \t\r\n");
  
  if (pos_start == npos || pos_end == npos) {
    str = "";
  }
  else {
    str = str.substr(pos_start, pos_end - pos_start + 1);
  }
}

void trim(uc_string& str) {
  uc_string res;
  
  size_t pos_start = str.find_first_not_of(U" \t\r\n");
  size_t pos_end = str.find_last_not_of(U" \t\r\n");
  
  if (pos_start == npos || pos_end == npos) {
    str = U"";
  }
  else {
    str = str.substr(pos_start, pos_end - pos_start + 1);
  }
}

void trimRight(uc_string& str) {
  uc_string res;
  
  size_t pos_start = 0;
  size_t pos_end = str.find_last_not_of(U" \t\r\n");
  
  if (pos_end == npos) {
    str = uc_string();
  }
  else {
    str = str.substr(pos_start, pos_end - pos_start + 1);
  }
}

string trimNewline(const string& str) {
  string res;
  
  size_t pos_start = str.find_first_not_of("\r\n");
  size_t pos_end = str.find_last_not_of("\r\n");
  
  if (pos_start == npos || pos_end == npos) {
    res = "";
  }
  else {
    res = str.substr(pos_start, pos_end - pos_start + 1);
  }
  
  return res;
}

void lineIndent(string& str) {
  if (str.find("\n") == string::npos) return;
  
  string res;
  
  size_t len = str.size();
  for (size_t pos = 0; pos < len; pos++) {
    if (str[pos] == '\r') continue;
    res.push_back(str[pos]);
    if (str[pos] == '\n') {
      res += "  ";
    }
  }
  
  str = res;
}

bool isXchar(uc_char c) {
  bool res = false;
  if (c >= Xchars::codePoints[0] && c <= Xchars::codePoints[Xchars::codePointsCount-1]) {
    res = std::find(std::begin(Xchars::codePoints), std::end(Xchars::codePoints), c) != std::end(Xchars::codePoints);
  }
  return res;
}

void fixUnicode(uc_string& str) {
  uc_string res;
  
  int len = str.size();
  for (int i = 0; i < len; i++) {
    uc_char c = str[i];
    
    if (isXchar(c)) {
      res += U"<span class='fx'>";
      res.push_back(c);
      res += U"</span>";
      // Log::w("-- detected broken char: [%d] %x", i, c);
    }
    else {
      res += c;
    }
  }
  
  str = res;
}

bool cmpWords(const string &s1, const string &s2) {
  uc_string us1 = Folding::apply(Utils::convert(s1));
  uc_string us2 = Folding::apply(Utils::convert(s2));
  return us1 < us2;
}


// ---------------------------------------- Utils ----------------------------------------

bool find(const vector<string>& container, const string& val) {
  return std::find(std::begin(container), std::end(container), val) != std::end(container);
}


// ---------------------------------------- File IO ----------------------------------------

void writeToFile(fstream& file, const uc_string& str) {
  file.write((char*) str.data(), str.size() * sizeof(uc_char));
  file.flush();
}

void writeToFile(fstream& file, const string& str) {
  file.write((char*) str.data(), str.size() * sizeof(char));
  file.flush();
}

bool fileExists(string fileName) {
  fstream f(fileName);
  return f.good();
}


void runTest() {
  Log::d("-----");
  fstream f;
  const char* filename = "/sdcard/HomeDictionary/_test2.txt";
  f.open(filename, ios_base::in | ios_base::binary);
  
  size_t size = 90*1024;
  char buf[size];
  f.read(buf, size);
  size_t dataSize = f.gcount();
  f.close();
  
  Log::d("dataSize: %d", dataSize);
  
  char buf2[400000];
  
  // uc_string str;
  // Utils::convert(str, buf, dataSize, "UCS-4LE", "UTF-16LE");
  // Log::d("Test string converted, str.size(): %d", str.size());
  // Log::d("-----");
}

}
// namespace
