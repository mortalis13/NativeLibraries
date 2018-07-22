#ifndef BTREEINDEX_HH
#define BTREEINDEX_HH

#include <string>
#include <vector>
#include <map>

#include "idxfile.h"
#include "vars.h"

using std::string;
using std::vector;
using std::map;
using std::pair;


const size_t BTREE_MIN_ELEMENTS = 64;
const size_t BTREE_MAX_ELEMENTS = 2048;

struct WordArticleLink {
  string word, prefix;
  uint32_t articleOffset;

  WordArticleLink() {}
  WordArticleLink(const string& word_, uint32_t articleOffset_, const string& prefix_ = string()) :
    word(word_), prefix(prefix_), articleOffset(articleOffset_) {}
};


class BtreeIndex {
  
  typedef pair<string, vector<WordArticleLink>> IndexMapPair;
  typedef map<string, vector<WordArticleLink>> IndexMap;
  

  IdxFile *idxReader;
  IndexMap indexedWordsMap;
  vector<char> rootNode;
  
  uint32_t indexNodeSize;
  uint32_t rootOffset;
  bool rootNodeLoaded;

public:

  BtreeIndex(): idxReader(0), rootNodeLoaded(false) {}

  void addWord(const uc_string& word, uint32_t articleOffset, unsigned int maxHeadwordSize = 256U);
  bool buildIndex(IdxFile& file, uint32_t& treeMaxElements, uint32_t& treeRootOffset);
  uint32_t buildTreeNode(IndexMap::const_iterator& nextIndex, size_t indexSize, IdxFile& file, size_t maxElements, uint32_t& lastLeafLinkOffset);
  
  // void openIndex(uint32_t indexBtreeMaxElements, uint32_t indexRootOffset, IdxFile& file);
  void openIndex(uint32_t indexBtreeMaxElements, uint32_t indexRootOffset, IdxFile*& file);
  const char* findChainOffset(const uc_string& target, bool& exactMatch, vector<char>& leaf, uint32_t& nextLeaf, const char*& leafEnd);
  void readNode(uint32_t offset, vector<char>& out);
  vector<WordArticleLink> readChain(const char*& ptr);
  
  void prefixMatch(const uc_string& str, vector<string>& mainMatches, vector<string>& fuzzyMatches, uint32_t maxResults);

  vector<uint32_t> findArticles(const uc_string& word);
  void antialias(const uc_string& str, vector<WordArticleLink>& chain);
  
  vector<string> getWords();
  
};

#endif
