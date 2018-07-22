#ifndef BASEDICTIONARY_HH
#define BASEDICTIONARY_HH

#include <string>
#include <vector>

#include "btreeindex.h"


using std::string;
using std::vector;


class BaseDictionary {
  
protected:
  
  BtreeIndex indexTree;

public:

  BaseDictionary() {}
  virtual ~BaseDictionary() {}

  // interface
  virtual string getName() = 0;
  virtual unsigned long getWordCount() = 0;
  virtual unsigned long getArticleCount() = 0;
  
  virtual uint32_t getLangFrom() const {
    return 0;
  }

  virtual uint32_t getLangTo() const {
    return 0;
  }
  
  void prefixMatch(const uc_string& str, vector<string>& mainMatches, vector<string>& fuzzyMatches, uint32_t maxResults);

};

#endif
