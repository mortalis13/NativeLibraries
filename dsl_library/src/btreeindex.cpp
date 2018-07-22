
#include "btreeindex.h"

#include <math.h>
#include <zlib.h>

#include "folding.h"
#include "utils.h"


void BtreeIndex::addWord(const uc_string& word, uint32_t articleOffset, unsigned int maxHeadwordSize) {
  const uc_char* wordBegin = word.c_str();
  size_t wordSize = word.size();

  if (wordSize > maxHeadwordSize) {
    Log::w("Skipped word with more than %d characters", maxHeadwordSize);
    return;
  }
  
  // find first and last non-space chars
  while (*wordBegin && Folding::isWhitespace(*wordBegin)) {
    ++wordBegin;
    --wordSize;
  }
  while (wordSize && Folding::isWhitespace(wordBegin[wordSize - 1])) {
    --wordSize;
  }

  const uc_char* nextChar = wordBegin;
  vector<char> utfBuffer(wordSize * sizeof(uc_char));
  int wordsAdded = 0; // Number of stored parts
  
  while (true) {
    while (true) {
      // End of string
      if (!*nextChar) {
        if (wordsAdded == 0) {
          uc_string folded = Folding::applyWhitespaceOnly(uc_string(wordBegin, wordSize));
          
          if (!folded.empty()) {
            string key = Utils::convertBytes(folded);
            vector<WordArticleLink> val;
            IndexMap::iterator it = indexedWordsMap.insert(IndexMapPair(key, val)).first;
            
            string utfWord = Utils::convertBytes(wordBegin, wordSize);
            string utfPrefix;
            it->second.reserve(it->second.size() + 1);
            it->second.push_back(WordArticleLink(utfWord, articleOffset, utfPrefix));
          }
        }
        
        return;
      }
  
      if (!Folding::isWhitespace(*nextChar) && !Folding::isPunct(*nextChar)) break;
      nextChar++;
    }

    string key = Utils::convertBytes(Folding::apply(nextChar));
    vector<WordArticleLink> val;
    IndexMap::iterator it = indexedWordsMap.insert(IndexMapPair(key, val)).first;

    if (it->second.size() < 1024 || nextChar == wordBegin) {
      // Don't overpopulate chains with middle matches
      string utfWord = Utils::convertBytes(nextChar, wordSize - (nextChar - wordBegin));
      string utfPrefix = Utils::convertBytes(wordBegin, nextChar - wordBegin);
      it->second.reserve(it->second.size() + 1);
      it->second.push_back(WordArticleLink(utfWord, articleOffset, utfPrefix));
    }

    wordsAdded++;

    while (true) {
      nextChar++;
      if (!*nextChar) return; // End of string
      if (Folding::isWhitespace(*nextChar) || Folding::isPunct(*nextChar)) break;
    }
  }
}

bool BtreeIndex::buildIndex(IdxFile& file, uint32_t& treeMaxElements, uint32_t& treeRootOffset) {
  size_t indexSize = indexedWordsMap.size();
  IndexMap::const_iterator nextIndex = indexedWordsMap.begin();

  while (indexSize && nextIndex->first.empty()) {
    indexSize--;
    ++nextIndex;
  }

  size_t maxElements = ((size_t) sqrt((double) indexSize)) + 1;
  maxElements = min( max(maxElements, BTREE_MIN_ELEMENTS), BTREE_MAX_ELEMENTS );
  
  Log::d("Building a tree of %u elements\n", (unsigned) maxElements);

  uint32_t lastLeafOffset = 0;
  uint32_t rootOffset = buildTreeNode(nextIndex, indexSize, file, maxElements, lastLeafOffset);
  
  indexedWordsMap.clear();
  
  treeMaxElements = maxElements;
  treeRootOffset = rootOffset;
  
  return true;
}


uint32_t BtreeIndex::buildTreeNode(IndexMap::const_iterator& nextIndex, size_t indexSize, IdxFile& file, size_t maxElements, uint32_t& lastLeafLinkOffset) {
  vector<unsigned char> uncompressedData;
  bool isLeaf = indexSize <= maxElements;

  if (isLeaf) {
    uint32_t totalChainsLength = 0;
    IndexMap::const_iterator nextWord = nextIndex;

    for (unsigned x = indexSize; x--; ++nextWord) {
      totalChainsLength += sizeof(uint32_t);
      const vector<WordArticleLink>& chain = nextWord->second;
      
      for (unsigned y = 0; y < chain.size(); ++y) {
        totalChainsLength += chain[y].word.size() + 1 + chain[y].prefix.size() + 1 + sizeof(uint32_t);
      }
    }

    uncompressedData.resize(sizeof(uint32_t) + totalChainsLength);
    *(uint32_t*)& uncompressedData.front() = indexSize;
    unsigned char* ptr = &uncompressedData.front() + sizeof(uint32_t);

    for (unsigned x = indexSize; x--; ++nextIndex) {
      const vector<WordArticleLink>& chain = nextIndex->second;
      unsigned char* saveSizeHere = ptr;
      ptr += sizeof(uint32_t);
      uint32_t size = 0;

      for (unsigned y = 0; y < chain.size(); ++y) {
        memcpy(ptr, chain[y].word.c_str(), chain[y].word.size() + 1);
        ptr += chain[y].word.size() + 1;

        memcpy(ptr, chain[y].prefix.c_str(), chain[y].prefix.size() + 1);
        ptr += chain[y].prefix.size() + 1;

        memcpy(ptr, &(chain[y].articleOffset), sizeof(uint32_t));
        ptr += sizeof(uint32_t);

        size += chain[y].word.size() + 1 + chain[y].prefix.size() + 1 + sizeof(uint32_t);
      }

      memcpy(saveSizeHere, &size, sizeof(uint32_t));
    }
  }
  else {
    uncompressedData.resize(sizeof(uint32_t) + (maxElements + 1) * sizeof(uint32_t));
    *(uint32_t*)& uncompressedData.front() = 0xffffFFFF;
    unsigned prevEntry = 0;

    for (unsigned x = 0; x < maxElements; ++x) {
      unsigned curEntry = (uint64_t) indexSize * (x + 1) / (maxElements + 1);

      uint32_t offset = buildTreeNode(nextIndex, curEntry - prevEntry, file, maxElements, lastLeafLinkOffset);
      memcpy(&uncompressedData.front() + sizeof(uint32_t) + x * sizeof(uint32_t), &offset, sizeof(uint32_t));
      size_t sz = nextIndex->first.size() + 1;
      size_t prevSize = uncompressedData.size();
      uncompressedData.resize(prevSize + sz);

      memcpy(&uncompressedData.front() + prevSize, nextIndex->first.c_str(), sz);
      prevEntry = curEntry;
    }

    uint32_t offset = buildTreeNode(nextIndex, indexSize - prevEntry, file, maxElements, lastLeafLinkOffset);
    memcpy(&uncompressedData.front() + sizeof(uint32_t) + maxElements * sizeof(uint32_t), &offset, sizeof(offset));
  }

  vector<unsigned char> compressedData(compressBound(uncompressedData.size()));
  unsigned long compressedSize = compressedData.size();
  bool res = compress(&compressedData.front(), &compressedSize, &uncompressedData.front(), uncompressedData.size());
  if (res != Z_OK) {
    throw runtime_error(Vars::ERROR_BTREE_COMPRESS_NODE);
  }

  uint32_t offset = file.tell();

  file.write((uint32_t) uncompressedData.size());
  file.write((uint32_t) compressedSize);
  file.write(&compressedData.front(), compressedSize);

  if (isLeaf) {
    file.write((uint32_t) 0);
    uint32_t here = file.tell();

    if (lastLeafLinkOffset) {
      file.seek(lastLeafLinkOffset);
      file.write(offset);
      file.seek(here);
    }

    lastLeafLinkOffset = here - sizeof(uint32_t);
  }

  return offset;
}

// void BtreeIndex::openIndex(uint32_t indexBtreeMaxElements, uint32_t indexRootOffset, IdxFile& file) {
void BtreeIndex::openIndex(uint32_t indexBtreeMaxElements, uint32_t indexRootOffset, IdxFile*& file) {
  indexNodeSize = indexBtreeMaxElements;
  rootOffset = indexRootOffset;

  // idxReader = &file;
  idxReader = file;

  rootNodeLoaded = false;
  rootNode.clear();
}

const char* BtreeIndex::findChainOffset(const uc_string& target, bool& exactMatch, vector<char>& extLeaf, uint32_t& nextLeaf, const char*& leafEnd) {
  if (!idxReader) throw runtime_error(Vars::ERROR_INDEX_NOT_OPENED);
  
  vector<uc_char> wcharBuffer;
  exactMatch = false;
  uint32_t currentNodeOffset = rootOffset;

  if (!rootNodeLoaded) {
    readNode(rootOffset, rootNode);
    rootNodeLoaded = true;
  }

  const char* leaf = &rootNode.front();
  leafEnd = leaf + rootNode.size();

  if (target.empty()) {
    while (true) {
      uint32_t leafEntries = *(uint32_t*) leaf;

      if (leafEntries == 0xffffFFFF) {
        currentNodeOffset = *((uint32_t*) leaf + 1);
        readNode(currentNodeOffset, extLeaf);
        leaf = &extLeaf.front();
        leafEnd = leaf + extLeaf.size();
        nextLeaf = idxReader->read<uint32_t>();
      }
      else {
        if (currentNodeOffset == rootOffset) {
          nextLeaf = 0;
        }
        
        if (!leafEntries) return 0;
        return leaf + sizeof(uint32_t);
      }
    }
  }

  while (true) {
    uint32_t leafEntries = *(uint32_t*) leaf;

    if (leafEntries == 0xffffFFFF) {
      const uint32_t* offsets = (uint32_t*) leaf + 1;
      const char* ptr = leaf + sizeof(uint32_t) + (indexNodeSize + 1) * sizeof(uint32_t);
      const char* closestString;
      int compareResult;

      const char* window = ptr;
      unsigned windowSize = leafEnd - ptr;

      while (true) {
        const char* testPoint = window + windowSize/2;
        closestString = testPoint;
  
        while (closestString > ptr && closestString[-1]) {
          --closestString;
        }
  
        size_t wordSize = strlen(closestString);
  
        if (wcharBuffer.size() <= wordSize) {
          wcharBuffer.resize(wordSize + 1);
        }
  
        long result = Utils::convertBytes(closestString, wordSize, &wcharBuffer.front());
        if (result < 0) throw runtime_error(Vars::ERROR_CONVERT_STRING + ": " + string(closestString));
  
        wcharBuffer[result] = 0;
        compareResult = target.compare(&wcharBuffer.front());
  
        if (!compareResult) {
          break;
        }
        else if (compareResult < 0) {
          windowSize = closestString - window;
          if (!windowSize) break;
        }
        else {
          windowSize -= (closestString - window) + wordSize + 1;
          window = closestString + wordSize + 1;
          if (!windowSize) break;
        }
      }

      unsigned entry = 0;
      for (const char* next = ptr; next != closestString; next += strlen(next) + 1) {
        entry++;
      }

      if (!compareResult) {
        currentNodeOffset = offsets[entry + 1];
      }
      else if (compareResult < 0) {
        currentNodeOffset = offsets[entry];
      }
      else {
        currentNodeOffset = offsets[entry + 1];
      }

      readNode(currentNodeOffset, extLeaf);
      leaf = &extLeaf.front();
      leafEnd = leaf + extLeaf.size();
    }
    else {
      nextLeaf = currentNodeOffset != rootOffset ? idxReader->read<uint32_t>() : 0;

      if (!leafEntries) {
        if (currentNodeOffset != rootOffset) throw runtime_error(Vars::ERROR_BTREE_CORRUPT_LEAF);
        else return 0; // No match
      }

      const char* ptr = leaf + sizeof(uint32_t);
      uint32_t chainSize;
      vector<const char*> chainOffsets(leafEntries);

      const char** nextOffset = &chainOffsets.front();
      while (leafEntries--) {
        *nextOffset++ = ptr;
        memcpy(&chainSize, ptr, sizeof(uint32_t));
        ptr += sizeof(uint32_t) + chainSize;
      }

      const char** window = &chainOffsets.front();
      unsigned windowSize = chainOffsets.size();

      while (true) {
        const char** chainToCheck = window + windowSize/2;
        ptr = *chainToCheck;
  
        memcpy(&chainSize, ptr, sizeof(uint32_t));
        ptr += sizeof(uint32_t);
  
        size_t wordSize = strlen(ptr);
  
        if (wcharBuffer.size() <= wordSize) {
          wcharBuffer.resize(wordSize + 1);
        }
  
        long result = Utils::convertBytes(ptr, wordSize, &wcharBuffer.front());
        if (result < 0) throw runtime_error(Vars::ERROR_CONVERT_STRING + ": " + string(ptr));
  
        wcharBuffer[result] = 0;
  
        uc_string foldedWord = Folding::apply(&wcharBuffer.front());
        if (foldedWord.empty()) foldedWord = Folding::applyWhitespaceOnly(&wcharBuffer.front());
        
        int compareResult = target.compare(foldedWord);
        
        if (!compareResult) {
          exactMatch = true;
          return ptr - sizeof(uint32_t);
        }
        else if (compareResult < 0) {
          windowSize /= 2;
          if (!windowSize) {
            return ptr - sizeof(uint32_t);
          }
        }
        else {
          windowSize -= windowSize/2 + 1;

          if (!windowSize) {
            if (chainToCheck == &chainOffsets.back()) {
              if (nextLeaf) {
                readNode(nextLeaf, extLeaf);
                leafEnd = &extLeaf.front() + extLeaf.size();
                nextLeaf = idxReader->read<uint32_t>();
                
                return &extLeaf.front() + sizeof(uint32_t);
              }
              else {
                return 0; // This was the last leaf
              }
            }
            else {
              return chainToCheck[1];
            }
          }

          window = chainToCheck + 1;
        }
      } // while
    }
  } // while
}

void BtreeIndex::readNode(uint32_t offset, vector<char>& out) {
  idxReader->seek(offset);

  uint32_t uncompressedSize = idxReader->read<uint32_t>();
  uint32_t compressedSize = idxReader->read<uint32_t>();

  out.resize(uncompressedSize);
  vector<unsigned char> compressedData(compressedSize);
  idxReader->read(&compressedData.front(), compressedData.size());
  unsigned long decompressedLength = out.size();

  bool res = uncompress((unsigned char*) &out.front(), &decompressedLength, &compressedData.front(), compressedData.size());
  if (res != Z_OK || decompressedLength != out.size()) {
    throw runtime_error(Vars::ERROR_BTREE_DECOMPRESS_NODE);
  }
}

vector<WordArticleLink> BtreeIndex::readChain(const char*& ptr) {
  uint32_t chainSize;

  memcpy(&chainSize, ptr, sizeof(uint32_t));
  ptr += sizeof(uint32_t);
  vector<WordArticleLink> result;

  while (chainSize) {
    string str = ptr;
    ptr += str.size() + 1;

    string prefix = ptr;
    ptr += prefix.size() + 1;

    uint32_t articleOffset;
    memcpy(&articleOffset, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    result.push_back(WordArticleLink(str, articleOffset, prefix));

    if (chainSize < str.size() + 1 + prefix.size() + 1 + sizeof(uint32_t)) {
      throw runtime_error(Vars::ERROR_BTREE_CORRUPT_LEAF);
    }
    else {
      chainSize -= str.size() + 1 + prefix.size() + 1 + sizeof(uint32_t);
    }
  }

  return result;
}


void BtreeIndex::prefixMatch(const uc_string& str, vector<string>& mainMatches, vector<string>& fuzzyMatches, uint32_t maxResults) {
  Log::d(U"BtreeIndex::prefixMatch(): " + str);
  
  uc_string folded = Folding::apply(str);
  if (folded.empty()) folded = Folding::applyWhitespaceOnly(str);

  bool exactMatch;
  vector<char> leaf;
  uint32_t nextLeaf;
  const char* leafEnd;
  
  const char* chainOffset = findChainOffset(folded, exactMatch, leaf, nextLeaf, leafEnd);

  if (chainOffset) {
    while (true) {
      vector<WordArticleLink> chain = readChain(chainOffset);
      uc_string chainHead = Utils::convertBytes(chain[0].word);

      uc_string resultFolded = Folding::apply(chainHead);
      if (resultFolded.empty()) resultFolded = Folding::applyWhitespaceOnly(chainHead);
      
      bool resultEqual = resultFolded.size() >= folded.size() && !resultFolded.compare(0, folded.size(), folded);
      if (folded.empty() || resultEqual) {
        unsigned len = chain.size();
        for (unsigned x = 0; x < len; ++x) {
          if (Config::STOP_SEARCH) return;
          
          string match = chain[x].prefix + chain[x].word;
          if (chain[x].prefix.empty()) {
            // Log::d("-- MAIN " + match);
            if (!Utils::find(mainMatches, match)) {
              mainMatches.push_back(match);
            }
          }
          else {
            // Log::d("-- FUZZ " + match);
            if (!Utils::find(fuzzyMatches, match) && (maxResults == 0 || fuzzyMatches.size() < maxResults)) {
              fuzzyMatches.push_back(match);
            }
          }
        }
      }
      else break;
      
      if (maxResults != 0 && mainMatches.size() >= maxResults) break;

      if (chainOffset >= leafEnd) {
        if (nextLeaf) {
          readNode(nextLeaf, leaf);
          leafEnd = &leaf.front() + leaf.size();

          nextLeaf = idxReader->read<uint32_t>();
          chainOffset = &leaf.front() + sizeof(uint32_t);

          uint32_t leafEntries = *(uint32_t*) &leaf.front();
          if (leafEntries == 0xffffFFFF) {
            throw runtime_error("leafEntries == 0xffffFFFF");
          }
        }
        else break; // That was the last leaf
      }
    } // for
  }
}


vector<uint32_t> BtreeIndex::findArticles(const uc_string& word) {
  vector<uint32_t> result;

  try {
    uc_string folded = Folding::apply(word);
    if (folded.empty()) folded = Folding::applyWhitespaceOnly(word);

    bool exactMatch;
    vector<char> leaf;
    uint32_t nextLeaf;
    const char* leafEnd;
    
    const char* chainOffset = findChainOffset(folded, exactMatch, leaf, nextLeaf, leafEnd);
    
    if (chainOffset && exactMatch) {
      vector<WordArticleLink> links = readChain(chainOffset);
      antialias(word, links);
      
      string wordUtf = Utils::convertBytes(word);
      for (auto link: links) {
        if (wordUtf.compare(link.prefix + link.word) == 0)
          result.push_back(link.articleOffset);
      }
    }
  }
  catch(std::exception & e) {
    Log::e("Articles searching failed, error: %s\n", e.what());
    result.clear();
  }
  catch(...) {
    Log::e("[ERROR]: Articles searching failed\n");
    result.clear();
  }

  return result;
}

void BtreeIndex::antialias(const uc_string& str, vector<WordArticleLink>& chain) {
  uc_string caseFolded = Folding::applySimpleCaseOnly(str);

  for (unsigned x = chain.size(); x--;) {
    uc_string ucWord = Utils::convertBytes(chain[x].prefix + chain[x].word);
    
    if (Folding::applySimpleCaseOnly(ucWord) != caseFolded) {
      chain.erase(chain.begin() + x);
    }
    else if (chain[x].prefix.size()) {
      // If there's a prefix, merge it with the word,
      chain[x].word.insert(0, chain[x].prefix);
      chain[x].prefix.clear();
    }
  }
}

vector<string> BtreeIndex::getWords() {
  vector<string> res;
  for (auto p: indexedWordsMap) {
    res.push_back(p.first);
  }
  return res;
}
