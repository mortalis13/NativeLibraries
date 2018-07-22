#ifndef DPE_H
#define DPE_H

#include <vector>

#include "vars.h"
#include "utils.h"

using std::string;
using std::vector;


namespace DPE {
  
struct DictInfo {
  string name;
  string direction;
  unsigned long headwordsCount;
  unsigned long articlesCount;
  bool indexError;
  string errorMsg;
};


DictInfo indexDict(const string& dictPath, const string& indexPath);
DictInfo scanAbbrevDict(const string& dictPath);
vector<string> searchWord(const string& dictPath, const string& indexPath, const uc_string& word, uint32_t maxResults = 30);
vector<string> searchWord(const vector<string>& dictFiles, const string& indexPath, const uc_string& word, uint32_t maxResults = 30);
uc_string getArticle(const string& dictPath, const string& indexPath, const uc_string& word);

uc_string getAbbrevDictHtml(const string& dictPath);

void stopIndexing();
void stopSearch();

}

#endif