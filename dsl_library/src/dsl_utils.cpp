#include "dsl_utils.h"

#include "folding.h"


namespace DslUtils {
  
  pos_t findSubentryChar(const uc_string& str) {
    uc_char ch = 0;
    size_t pos = 0;
    
    do {
      ch = str[pos];
      if (!Utils::isWhitespace(ch)) {
        if (ch == U'@') return pos;
        else return npos;
      }
      pos++;
    }
    while (ch);
    
    return npos;
  }
  
  bool isSubentryClose(const uc_string& s) {
    uc_string s1 = Folding::trimWhitespace(s);
    return s1.length() == 1;
  }
  
  bool isHeaderStart(char b1, char b2) {
    // #
    return b1 == 0x23 && b2 == 0x00;
  }
  
  bool isHeadwordStart(char b1, char b2) {
    // \t, ' ', \r, \n
    return (b1 != 0x09 && b1 != 0x20 && b1 != 0x0d && b1 != 0x0a) || b2 != 0x00;
  }
  
  bool isHeaderStart(char b1) {
    return isHeaderStart(b1, 0x00);
  }
  
  bool isHeadwordStart(char b1) {
    return isHeadwordStart(b1, 0x00);
  }
  
  void processUnsortedParts(uc_string& str, bool strip) {
    int refCount = 0;
    size_t startPos = 0;

    for (size_t x = 0; x < str.size();) {
      uc_char ch = str[x];
      if (ch == U'\\') {
        x += 2;
        continue;
      }

      if (ch == '{') {
        ++refCount;

        if (!strip) {
          str.erase(x, 1);
          continue;
        }
        else if (refCount == 1) {
          startPos = x;
        }
      }
      else if (ch == '}') {
        --refCount;

        if (refCount < 0) {
          Log::w("[WARNING]: unmatched closing braces encountered.\n");
          
          refCount = 0;
          str.erase(x, 1);
          continue;
        }

        if (!strip) {
          str.erase(x, 1);
          continue;
        }
        else if (!refCount) {
          str.erase(startPos, x - startPos + 1);
          x = startPos;
          continue;
        }
      }

      ++x;
    }

    if (strip && refCount) {
      string strRes;
      Utils::convert(strRes, str);
      Log::w("[WARNING]: unclosed braces encountered: '" + strRes + "'");
      str.erase(startPos);
    }
  }

  void expandOptionalParts(uc_string& str, list<uc_string>* result, size_t x, bool inside_recurse) {
    list<uc_string> expanded;
    list<uc_string>* headwords = inside_recurse ? result: &expanded;

    while (x < str.size()) {
      uc_char ch = str[x];
      if (ch == U'\\') {
        x += 2;
      }
      else if (ch == U'(') {
        int refCount = 1;

        for (size_t y = x + 1; y < str.size(); ++y) {
          uc_char ch = str[y];
          if (ch == U'\\') {
            ++y;
          }
          else if (ch == U'(') {
            ++refCount;
          }
          else if (ch == U')' && !--refCount) {
            if (y != x + 1) {
              uc_string removed(str, 0, x);
              removed.append(str, y + 1, str.size() - y - 1);
              expandOptionalParts(removed, headwords, x, true);
            }

            break;
          }
        }

        if (refCount && x != str.size() - 1) {
          uc_string removed(str, 0, x);
          if (headwords->size() < 32) {
            headwords->push_back(removed);
          }
          else {
            if (!inside_recurse) {
              result->merge(expanded);
            }
            return;
          }
        }

        str.erase(x, 1);
      }
      else if (ch == U')') {
        str.erase(x, 1);
      }
      else {
        ++x;
      }
    }

    if (headwords->size() < 32) headwords->push_back(str);
    if (!inside_recurse) result->merge(expanded);
  }

  void stripComments(uc_string& str, bool& nextLine) {
    pos_t n = 0, n2 = 0;

    while (true) {
      if (nextLine) {
        n = str.find(U"}}", n2);
        if (n == npos) {
          str.erase(n2, n);
          break;
        }
        
        str.erase(n2, n - n2 + 2);
        nextLine = false;
      }

      n = str.find(U"{{", n2);
      if (n == npos) break;
      nextLine = true;
      n2 = n;
    }
  }

  void stripComments(string& str, bool& nextLine) {
    pos_t n = 0, n2 = 0;

    while (true) {
      if (nextLine) {
        n = str.find("}}", n2);
        if (n == npos) {
          str.erase(n2, n);
          break;
        }
        
        str.erase(n2, n - n2 + 2);
        nextLine = false;
      }

      n = str.find("{{", n2);
      if (n == npos) break;
      nextLine = true;
      n2 = n;
    }
  }


  void expandTildes(uc_string& str, const uc_string& tildeReplacement) {
    uc_string tildeValue = Folding::trimWhitespace(tildeReplacement);
    
    for (size_t x = 0; x < str.size();) {
      if (str[x] == U'\\') {
        x += 2;
      }
      else if (str[x] == U'~') {
        if (x > 0 && str[x - 1] == '^' && (x < 2 || str[x - 2] != '\\')) {
          str.replace(x - 1, 2, tildeValue);
          // !! not done [convert case for the str[x-1] character, towupper() doesn't work for non-English codes]
          x = x - 1 + tildeValue.size();
        }
        else {
          str.replace(x, 1, tildeValue);
          x += tildeValue.size();
        }
      }
      else {
        ++x;
      }
    }
  }

  void unescapeDsl(uc_string& str) {
    for (size_t x = 0; x < str.size(); ++x) {
      if (str[x] == U'\\') str.erase(x, 1);
    }
  }

  void normalizeHeadword(uc_string& str) {
    for (size_t x = str.size(); x-- > 1;) {
      if (str[x] == U' ') {
        size_t y;
        for (y = x; y && str[y - 1] == U' '; ) {
          --y;
        }

        if (y != x) {
          str.erase(y, x - y);
          x = y;
        }
      }
    }
    
    if (!str.empty() && str[str.size() - 1] == U' ') str.erase(str.size() - 1, 1);
    if (!str.empty() && str[0] == U' ') str.erase(0, 1);
  }
  
  uc_string getLineLevelTag(const uc_string& str) {
    uc_string res;
    
    pos_t len = str.size();
    pos_t st_pos;
    for (st_pos = 0; st_pos < len; st_pos++) {
      if ( !Utils::isWhitespace(str[st_pos]) ) break;
    }
    
    if (len > 4 + st_pos) {
      uc_string tag = str.substr(st_pos, 4);
      if (tag == U"[m0]" || tag == U"[m1]" || tag == U"[m2]" || tag == U"[m3]" || 
          tag == U"[m4]" || tag == U"[m5]" || tag == U"[m6]" || tag == U"[m7]" || 
          tag == U"[m8]" || tag == U"[m9]")
      {
        res = tag;
      }
    }
    
    return res;
  }
  
  
  void processSubentries(uc_string& str) {
    uc_string res, lineStr, prevLine, prevPart;
    
    pos_t pos = 0;
    pos_t line_end_pos = 0;
    pos_t part_start = 0;
    pos_t part_end = 0;
    
    bool subentryStarted = false;
    
    size_t len = str.size();
    while (pos < len) {
      line_end_pos = str.find_first_of(U"\r\n", pos);
      lineStr = str.substr(pos, line_end_pos - pos);
      
      pos_t subentryCharPos = findSubentryChar(lineStr);
      if (subentryCharPos != npos) {
        if (isSubentryClose(lineStr)) {
          subentryStarted = false;
          
          part_start = pos + subentryCharPos + 1;
          if (str[part_start] == U'\r') part_start++;
          if (str[part_start] == U'\n') part_start++;
        }
        else {
          if (subentryStarted) part_start = pos;
          subentryStarted = true;
          
          part_end = pos + subentryCharPos;
          prevPart = str.substr(part_start, part_end - part_start);
          res += prevPart;
          
          uc_string subHeadword = Folding::trimWhitespace(lineStr.substr(subentryCharPos + 1));
          
          list<uc_string> subHeadwords;
          DslUtils::processUnsortedParts(subHeadword, true);
          DslUtils::expandOptionalParts(subHeadword, &subHeadwords);
          
          uc_string prevLevelTag = getLineLevelTag(prevLine);
          
          for (auto word: subHeadwords) {
            word = U"[ref]" + word + U"[/ref]";
            if (!prevLevelTag.empty()) word = prevLevelTag + word + U"[/m]";
            res += word + U"\n";
          }
        }
      }
      
      if (!subentryStarted) prevLine = lineStr;
      
      pos = line_end_pos;
      if (str[pos] == U'\r') pos++;
      if (str[pos] == U'\n') pos++;
    }
    
    if (!subentryStarted && part_start != 0 && part_start < len) {
      res += str.substr(part_start);
    }
    
    if (!res.empty()) str = res;
  }
  
}
