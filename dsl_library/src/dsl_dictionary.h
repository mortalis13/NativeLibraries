#ifndef DSL_DICTIONARY_H
#define DSL_DICTIONARY_H

#include <string>
#include <vector>
#include <list>

#include "btreeindex.h"
#include "basedictionary.h"
#include "chunkedstorage.h"
#include "folding.h"

#include "dsl_reader.h"
#include "dictzip.h"
#include "vars.h"

using std::string;
using std::vector;
using std::list;


struct IdxHeader {
  uint32_t chunksOffset;
  uint32_t indexBtreeMaxElements;
  uint32_t indexRootOffset;
  uint32_t articleCount;
  uint32_t wordCount;
  uint32_t langFrom;
  uint32_t langTo;
  uint32_t sourceEncoding;
};

struct SubEntry {
  uint32_t offset;
  uint32_t size;
  vector<uc_string> headwords;
  
  SubEntry(uint32_t _offset, uint32_t _size, const vector<uc_string>& words): 
    offset(_offset), size(_size), headwords(words) {}
};


class DslDictionary: public BaseDictionary {
  
  string dictPath;
  string indexPath;
  
  dictData *dz;
  IdxFile *idxReader;
  ChunkedStorage::Reader *chunksReader;
  IdxHeader idxHeader;
  
  string dictionaryName;
  string langFromStr;
  string langToStr;
  
  vector<uc_string> allWords;

private:
  
  template<typename T> void addWordsToIndex(const T& words, uint32_t offset);
  void addSubEntry(vector<SubEntry>& subEntries, size_t subentryOffset, size_t lineOffset, vector<uc_string>& subHeadwords);
  
  void loadArticle(uint32_t address, const uc_string& wordFolded, uc_string& tildeValue, uc_string& displayedHeadword, unsigned& headwordIndex, uc_string& articleText);

public:

  DslDictionary(const string& dictionaryFile);
  DslDictionary(const string& dictionaryFile, const string& indexFile);
  ~DslDictionary();
  
  // override
  virtual string getName() {
    return dictionaryName;
  }

  virtual unsigned long getWordCount() {
    return idxHeader.wordCount;
  }

  virtual unsigned long getArticleCount() {
    return idxHeader.articleCount;
  }

  inline virtual uint32_t getLangFrom() const {
    return idxHeader.langFrom;
  }

  inline virtual uint32_t getLangTo() const {
    return idxHeader.langTo;
  }
  
  
  string getDirection() {
    return langFromStr + langToStr;
  }
  
  bool indexDict(string& errorMsg);
  bool scanAbbrevDict(string& errorMsg);
  void indexDict2();
  void readIndex();
  uc_string getArticle(const uc_string& word);
  
  uc_string getAbbrevHtml();


  vector<uc_string> getAllWords() {
    return allWords;
  }
  
  void dumpIndexedWords();

};

#endif