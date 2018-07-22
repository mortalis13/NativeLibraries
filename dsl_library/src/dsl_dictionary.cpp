
#include "dsl_dictionary.h"

#include <list>
#include <stdlib.h>

#include "dsl_utils.h"
#include "utils.h"


DslDictionary::DslDictionary(const string& dictionaryFile, const string& indexFile) :
    dictPath(dictionaryFile),
    indexPath(indexFile),
    dz(0), idxReader(0), chunksReader(0)
{}

DslDictionary::DslDictionary(const string& dictionaryFile) :
    dictPath(dictionaryFile),
    dz(0), idxReader(0), chunksReader(0)
{}

DslDictionary::~DslDictionary() {
  if (dz) dict_data_close(dz);
  if (idxReader != NULL) {
    idxReader->close();
    delete idxReader;
  }
  if (chunksReader != NULL) {
    delete chunksReader;
  }
}


bool DslDictionary::indexDict(string& errorMsg) {
  Log::d("indexDict()");
  clock_t start_time = clock();
  
  size_t dslLine = 0;
  
  try {
    DslReader dslReader(dictPath);
    dslReader.readHeader();
    if (!dslReader.isDslFile()) throw runtime_error(Vars::ERROR_INCORRECT_DSL_FILE);
    dslReader.skipEmptyLines();
    
    IdxFile idxWriter(indexPath.c_str(), "wb");
    memset(&idxHeader, 0, sizeof(idxHeader));
    
    Utils::convert(dictionaryName, dslReader.getDictionaryName());
    
    idxWriter.write(idxHeader);
    idxWriter.write((uint32_t) dictionaryName.size());
    idxWriter.write(dictionaryName.data(), dictionaryName.size());
    idxHeader.sourceEncoding = dslReader.getSourceEncoding();
    
    ChunkedStorage::Writer chunksWriter(idxWriter);
    
    uc_string lineStr;
    size_t lineOffset;
    
    uc_string prevMainHeadword;
    uint32_t articleOffset = 0;
    uint32_t descOffset = 0;
    
    uint32_t articleCount = 0;
    uint32_t wordCount = 0;
    
    vector<SubEntry> subEntries;
    vector<uc_string> subHeadwords;
    uint32_t subentryStartLine = 0;
    uint32_t subentryOffset = 0;
    uint32_t prevBodyOffset = 0;
    bool subentryStarted = false;
    
    bool hasString = true;
    bool bodyEntered = false;
    
    while (hasString) {
      if (Config::STOP_INDEXING) return false;
      
      hasString = dslReader.readLine(lineStr, lineOffset);
      dslLine = dslReader.getLinesRead();
      
      bool isHeadword = lineStr.size() && !Utils::isWhitespace(lineStr[0]);
      bool isMainHeadword = isHeadword && (wordCount == 0 || bodyEntered);
      bool isNextHeadword = isMainHeadword && wordCount > 0;
      bool isIndexEnd = !hasString;
      
      // write articles size when reached the next headword or at the end for the last article
      // write subentries of current article after the article data is saved
      if (isNextHeadword || isIndexEnd) {
        uint32_t articleSize = lineOffset - articleOffset;
        chunksWriter.addToBlock(articleSize);
        
        if (subentryStarted) {
          Log::w("[WARNING]: Unclosed subentry (no closing @), getting subentry until the end of the current card, line: %d", subentryStartLine);
          subentryStarted = false;
          addSubEntry(subEntries, subentryOffset, lineOffset, subHeadwords);
        }
        
        for (auto subEntry: subEntries) {
          uint32_t subDescOffset = chunksWriter.startNewBlock();
          chunksWriter.addToBlock(subEntry.offset);
          chunksWriter.addToBlock(subEntry.size);
          
          addWordsToIndex(subEntry.headwords, subDescOffset);
          wordCount += subEntry.headwords.size();
          articleCount++;
        }
        subEntries.clear();
      }
      
      if (Utils::isEmpty(lineStr)) continue;
      bodyEntered = false;
      
      // 2 types of lines (headwords without initial spaces, and article body lines starting with tab or space)
      if (isHeadword) {
        list<uc_string> headwords;
        DslUtils::processUnsortedParts(lineStr, true);
        
        // first headword or additional headwords that come after the main one but before the article body
        if (isMainHeadword) {
          prevMainHeadword = lineStr;
          articleOffset = lineOffset;

          descOffset = chunksWriter.startNewBlock();
          chunksWriter.addToBlock(articleOffset);
          
          articleCount++;
        }
        else {
          DslUtils::expandTildes(lineStr, prevMainHeadword);
        }
        
        DslUtils::expandOptionalParts(lineStr, &headwords);
        
        addWordsToIndex(headwords, descOffset);
        wordCount += headwords.size();
      }
      else {
        bodyEntered = true;
        bool isVariantHeadword = lineOffset == prevBodyOffset;
        
        // find subentries (with @ and a headword at the start and @ at the end at the same indent level as the article body)
        size_t subentryCharPos = DslUtils::findSubentryChar(lineStr);
        if (subentryCharPos != npos) {
          // a subentry was started at some previous line
          // now @ closes the subentry body and its data is stored in the list which will be saved after the current main article data
          if (subentryStarted && !isVariantHeadword) {
            subentryStarted = false;
            addSubEntry(subEntries, subentryOffset, lineOffset, subHeadwords);
          }
          
          // found @ with a headword, put the headword to the list
          if ((!subentryStarted || isVariantHeadword) && !DslUtils::isSubentryClose(lineStr)) {
            subentryStarted = true;
            subentryStartLine = dslReader.getLinesRead();
            
            uc_string subHeadword = Folding::trimWhitespace(lineStr.substr(subentryCharPos + 1));
            if (!isVariantHeadword) subentryOffset = lineOffset;
            prevBodyOffset = dslReader.getFileOffset();
            
            list<uc_string> headwords;
            
            DslUtils::processUnsortedParts(subHeadword, true);
            DslUtils::expandTildes(subHeadword, prevMainHeadword);
            DslUtils::expandOptionalParts(subHeadword, &headwords);
            
            for (auto word: headwords) subHeadwords.push_back(word);
          }
        }
        
      } // if-else isHeadword
    } // while
    
    if (!bodyEntered) {
      Log::w("[WARNING]: Last headword has no body");
    }
    
    // dumpIndexedWords();
    
    // -- Finish index
    idxHeader.chunksOffset = chunksWriter.finish();
    indexTree.buildIndex(idxWriter, idxHeader.indexBtreeMaxElements, idxHeader.indexRootOffset);
    
    idxHeader.articleCount = articleCount;
    idxHeader.wordCount = wordCount;
    
    Utils::getLangInfo(dslReader.getLangFrom(), idxHeader.langFrom, langFromStr);
    Utils::getLangInfo(dslReader.getLangTo(), idxHeader.langTo, langToStr);
    
    idxWriter.rewind();
    idxWriter.write(&idxHeader, sizeof(idxHeader));
    
    idxWriter.close();
  }
  catch (std::exception & e) {
    Log::e("[ERROR]: DSL file reading: %s:%u. Details: %s\n", dictPath.c_str(), dslLine, e.what());
    errorMsg = e.what();
    return false;
  }
  
  clock_t end = clock();
  double diff = double(end - start_time) / CLOCKS_PER_SEC;
  Log::d("== Time, Indexing: %.2f", diff);
  
  Log::d("// indexDict()\n");
  return true;
}


bool DslDictionary::scanAbbrevDict(string& errorMsg) {
  Log::d("scanAbbrevDict()");
  
  try {
    DslReader dslReader(dictPath);
    dslReader.readHeader();
    if (!dslReader.isDslFile()) throw runtime_error(Vars::ERROR_INCORRECT_DSL_FILE);
    
    Utils::convert(dictionaryName, dslReader.getDictionaryName());
    
    idxHeader.articleCount = 0;
    idxHeader.wordCount = 0;
    
    Utils::getLangInfo(dslReader.getLangFrom(), idxHeader.langFrom, langFromStr);
    Utils::getLangInfo(dslReader.getLangTo(), idxHeader.langTo, langToStr);
  }
  catch (std::exception & e) {
    Log::e("[ERROR]: DSL file reading: %s. Details: %s\n", dictPath.c_str(), e.what());
    errorMsg = e.what();
    return false;
  }
  
  return true;
}


void DslDictionary::indexDict2() {
  Log::d("indexDict2()");
  clock_t start_time = clock();
  unsigned dslLine = 0;
  
  try {
    // -- Prepare data
    DslReader dslReader(dictPath);
    dslReader.readHeader();
    
    IdxFile idxWriter(indexPath.c_str(), "wb");
    memset(&idxHeader, 0, sizeof(idxHeader));
    
    idxWriter.write(idxHeader);
    Utils::convert(dictionaryName, dslReader.getDictionaryName());
    idxWriter.write((uint32_t) dictionaryName.size());
    idxWriter.write(dictionaryName.data(), dictionaryName.size());
    
    idxHeader.sourceEncoding = dslReader.getSourceEncoding();
    
    ChunkedStorage::Writer chunksWriter(idxWriter);
    
    bool hasString = false;
    uc_string lineStr;
    size_t lineOffset;
    uint32_t articleCount = 0;
    uint32_t wordCount = 0;
    
    // skip empty lines after header
    dslReader.skipEmptyLines();
    dslReader.readLine(lineStr, lineOffset);
    
    // -- Main Loop, collect headings
    for (;;) {
      dslLine = dslReader.getLinesRead() + 1;
      // -- lineStr has a headword here, lineOffset - the headword offset in the file --
      
      // hasString = false; ?
      if (Utils::isEmpty(lineStr)) continue;

      list<uc_string> allEntryWords;
      DslUtils::processUnsortedParts(lineStr, true);
      DslUtils::expandOptionalParts(lineStr, &allEntryWords);
      
      uint32_t articleOffset = lineOffset;
      
      // Searching for 
      for (;;) {
        hasString = dslReader.readLine(lineStr, lineOffset);             // [READ] reads variant headwords until the first body line (including)
        if (!hasString) {
          Log::e("Premature end of file %s\n", dictPath.c_str());
          break;
        }

        if (lineStr.empty()) continue;
        if (Utils::isWhitespace(lineStr[0])) break;

        // Log::d(U"--Alt headword:" + lineStr);
        DslUtils::processUnsortedParts(lineStr, true);
        DslUtils::expandTildes(lineStr, allEntryWords.front());
        DslUtils::expandOptionalParts(lineStr, &allEntryWords);
      }

      if (!hasString) break;

      // Insert new entry
      uint32_t descOffset = chunksWriter.startNewBlock();
      chunksWriter.addToBlock(&articleOffset, sizeof(articleOffset));
      
      // Log::d("descOffset: %d", descOffset);
      // Log::d("articleOffset: %d", articleOffset);
      
      for (auto ptrHeading = allEntryWords.begin(); ptrHeading != allEntryWords.end(); ++ptrHeading) {
        DslUtils::unescapeDsl(*ptrHeading);
        DslUtils::normalizeHeadword(*ptrHeading);
        indexTree.addWord(*ptrHeading, descOffset);
      }

      ++articleCount;
      wordCount += allEntryWords.size();

      uc_string headword;
      
      int headwordLine = dslReader.getLinesRead() - 2;
      int subentryLine = 0;
      bool noSignificantLines = Folding::applyWhitespaceOnly(lineStr).empty();
      bool wasEmptyLine = false;

      vector<SubEntry> subEntries;
      vector<uc_string> subHeadwords;
      unsigned subentryLines = 0;
      int insideSubentry = 0;
      
      uint32_t offset = lineOffset;
      
      // Skip article body
      for (;;) {
        hasString = dslReader.readLine(lineStr, lineOffset);                               // [READ] reads body lines until the next headword (including)
        // break if line not read or if headword (without first space)
        bool isHeadword = lineStr.size() && !Utils::isWhitespace(lineStr[0]);
        if (!hasString || isHeadword) {
          if (insideSubentry) {
            Log::w("Unclosed tag '@' at line %i", subentryLine);
            subEntries.push_back(SubEntry(offset, lineOffset - offset, subHeadwords));
          }
          if (noSignificantLines) Log::w("Orphan headword at line %i", headwordLine);
          
          break;
        }
        
        // -- inside body (line number > 1) --
        // Check for orphan strings
        if (lineStr.empty()) {
          wasEmptyLine = true;
          continue;
        }
        else if (wasEmptyLine && !Folding::applyWhitespaceOnly(lineStr).empty()) {
          Log::w("Orphan string at line %i", dslReader.getLinesRead());
        }

        if (noSignificantLines) noSignificantLines = Folding::applyWhitespaceOnly(lineStr).empty();

        // Find embedded cards
        size_t n = lineStr.find(U'@');
        if (n == npos || lineStr[n-1] == U'\\') {
          if (insideSubentry) subentryLines++;
          continue;
        }
        else if (DslUtils::findSubentryChar(lineStr) == npos) {
          // Embedded card tag must be placed at first position in line after spaces
          Log::w("Unescaped '@' symbol at line %i", dslReader.getLinesRead());
          if (insideSubentry) subentryLines++;
          continue;
        }
        
        // -- subentry found --
        subentryLine = dslReader.getLinesRead();

        // Handle embedded card
        if (insideSubentry) {
          if (subentryLines) {
            subEntries.push_back(SubEntry(offset, lineOffset - offset, subHeadwords));
            subHeadwords.clear();
            subentryLines = 0;
            offset = lineOffset;
          }
        }
        else {
          offset = lineOffset;
          subentryLines = 0;
        }

        headword = Folding::trimWhitespace(lineStr.substr(n + 1));

        if (!headword.empty()) {
          DslUtils::processUnsortedParts(headword, true);
          DslUtils::expandTildes(headword, allEntryWords.front());
          subHeadwords.push_back(headword);
          insideSubentry = true;
        }
        else {
          insideSubentry = false;
        }
      }

      uint32_t articleSize = lineOffset - articleOffset;
      chunksWriter.addToBlock(&articleSize, sizeof(articleSize));
      // Log::d("articleSize: %d", articleSize);
      
      for (auto ptrCard = subEntries.begin(); ptrCard != subEntries.end(); ++ptrCard) {
        uint32_t descOffset = chunksWriter.startNewBlock();
        chunksWriter.addToBlock(&(*ptrCard).offset, sizeof((*ptrCard).offset));
        chunksWriter.addToBlock(&(*ptrCard).size, sizeof((*ptrCard).size));

        for (size_t x = 0; x < (*ptrCard).headwords.size(); x++) {
          allEntryWords.clear();
          DslUtils::expandOptionalParts((*ptrCard).headwords[x], &allEntryWords);

          for (auto ptrHeading = allEntryWords.begin(); ptrHeading != allEntryWords.end(); ++ptrHeading) {
            DslUtils::unescapeDsl(*ptrHeading);
            DslUtils::normalizeHeadword(*ptrHeading);
            indexTree.addWord(*ptrHeading, descOffset);
          }

          wordCount += allEntryWords.size();
        }
        ++articleCount;
      }

      if (!hasString) break;
    }
    // End Main Loop
    
    // dumpIndexedWords();
    
    // -- Finish index
    idxHeader.chunksOffset = chunksWriter.finish();
    indexTree.buildIndex(idxWriter, idxHeader.indexBtreeMaxElements, idxHeader.indexRootOffset);
    
    idxHeader.articleCount = articleCount;
    idxHeader.wordCount = wordCount;
    
    Utils::getLangInfo(dslReader.getLangFrom(), idxHeader.langFrom, langFromStr);
    Utils::getLangInfo(dslReader.getLangTo(), idxHeader.langTo, langToStr);
    
    idxWriter.rewind();
    idxWriter.write(&idxHeader, sizeof(idxHeader));
  }
  catch (std::exception & e) {
    Log::e("[ERROR]: DSL dictionary reading failed: %s:%u, error: %s\n", dictPath.c_str(), dslLine, e.what());
  }
  
  clock_t end = clock();
  double diff = double(end - start_time) / CLOCKS_PER_SEC;
  Log::d("== Time, Indexing: %.2f", diff);
  
  Log::d("// indexDict()");
}

void DslDictionary::readIndex() {
  idxReader = new IdxFile(indexPath.c_str(), "rb");
  idxReader->read<IdxHeader>(idxHeader);
  idxReader->seek(sizeof(idxHeader));

  vector<char> dictName(idxReader->read<uint32_t>());
  idxReader->read(&dictName.front(), dictName.size());
  dictionaryName = string(&dictName.front(), dictName.size());

  chunksReader = new ChunkedStorage::Reader(*idxReader, idxHeader.chunksOffset);

  DZ_ERRORS error;
  dz = dict_data_open(dictPath.c_str(), &error, 0);
  if (!dz) throw runtime_error(Vars::ERROR_DICTZIP + ": " + string(dz_error_str(error)) + " (" + dictPath + ")");

  // indexTree.openIndex(idxHeader.indexBtreeMaxElements, idxHeader.indexRootOffset, *idxReader);
  indexTree.openIndex(idxHeader.indexBtreeMaxElements, idxHeader.indexRootOffset, idxReader);
}

void DslDictionary::loadArticle(uint32_t address, const uc_string& wordFolded, uc_string& tildeValue, uc_string& displayedHeadword, unsigned& headwordIndex, uc_string& articleText) {
  uc_string articleData;

  vector<char> chunk;
  uint32_t articleOffset, articleSize;
  char* articleProps = chunksReader->getBlock(address, chunk);

  memcpy(&articleOffset, articleProps, sizeof(articleOffset));
  memcpy(&articleSize, articleProps + sizeof(articleOffset), sizeof(articleSize));
  
  char* articleBody = dict_data_read_(dz, articleOffset, articleSize, 0, 0);
  
  if (!articleBody) {
    Log::e("[ERROR]: DICTZIP error: " + string(dict_error_str(dz)));
  }
  else {
    try {
      Encoding fromEnc = (Encoding) idxHeader.sourceEncoding;
      bool res = Utils::convert(articleData, articleBody, articleSize, ENC_UCS4LE, fromEnc);
      // Log::d(U"RAW_ARTICLE:\n''" + articleData + U"''");
      free(articleBody);
      
      bool b = false;
      DslUtils::stripComments(articleData, b);
    }
    catch (...) {
      free(articleBody);
      throw runtime_error("loadArticle() exception");
    }
  }
  
  size_t pos = 0;
  bool hadFirstHeadword = false;
  bool foundDisplayedHeadword = false;

  bool isSubEntry = Utils::isWhitespace(articleData.at(0));
  uc_string tildeValueWithUnsorted; // This one has unsorted parts left
  
  headwordIndex = 0;
  while (true) {
    size_t begin = pos;

    pos = articleData.find_first_of(U"\r\n", begin);
    if (pos == npos) pos = articleData.size();

    if (!foundDisplayedHeadword) {
      // Process the headword
      uc_string rawHeadword = uc_string(articleData, begin, pos - begin);

      if (isSubEntry && !rawHeadword.empty() && Utils::isWhitespace(rawHeadword[0])) {
        // Headword of the subentry
        size_t hpos = rawHeadword.find(U'@');
        if (hpos != npos) {
          uc_string head = Folding::trimWhitespace(rawHeadword.substr(hpos + 1));
          hpos = head.find(U'~');
          
          while (hpos != npos) {
            if (hpos == 0 || head[hpos] != U'\\') break;
            hpos = head.find(U'~', hpos + 1);
          }
          
          if (hpos == npos) rawHeadword = head;
          else rawHeadword.clear();
        }
      }

      if (!rawHeadword.empty()) {
        if (!hadFirstHeadword) {
          tildeValue = rawHeadword;
          list<uc_string> wordsList;

          DslUtils::expandOptionalParts(tildeValue, &wordsList);
          if (wordsList.size()) {
            tildeValue = wordsList.front();
          }
          
          tildeValueWithUnsorted = tildeValue;
          DslUtils::processUnsortedParts(tildeValue, false);
        }
        
        uc_string str = rawHeadword;

        if (hadFirstHeadword) DslUtils::expandTildes(str, tildeValueWithUnsorted);
        DslUtils::processUnsortedParts(str, true);
        str = Folding::applySimpleCaseOnly(str);

        list<uc_string> wordsList;
        DslUtils::expandOptionalParts(str, &wordsList);

        for (list<uc_string>::iterator ptrWord = wordsList.begin(); ptrWord != wordsList.end(); ++ptrWord) {
          DslUtils::unescapeDsl(*ptrWord);
          DslUtils::normalizeHeadword(*ptrWord);

          if (Folding::trimWhitespace(*ptrWord) == wordFolded) {
            if (hadFirstHeadword) {
              DslUtils::expandTildes(rawHeadword, tildeValueWithUnsorted);
            }

            DslUtils::processUnsortedParts(rawHeadword, false);
            displayedHeadword = rawHeadword;

            foundDisplayedHeadword = true;
            break;
          }
        }

        if (!foundDisplayedHeadword) {
          ++headwordIndex;
          hadFirstHeadword = true;
        }
      }
    }

    if (pos == articleData.size()) break;
    if (articleData[pos] == '\r') ++pos;
    if (pos != articleData.size() && articleData[pos] == '\n') ++pos;
    if (pos == articleData.size()) break;
    
    if (Utils::isWhitespace(articleData[pos])) {
      // Check for begin article text
      if (isSubEntry) {
        // Check for next subentry headword
        size_t hpos = articleData.find_first_of(U"\r\n", pos);
        if (hpos == npos) {
          hpos = articleData.size();
        }

        uc_string str = uc_string(articleData, pos, hpos - pos);
        hpos = str.find(U'@');
        if (hpos == npos || str[hpos - 1] == U'\\' || DslUtils::findSubentryChar(str) == npos) break;
      }
      else break;
    }
  }

  if (!foundDisplayedHeadword) {
    if (isSubEntry) {
      displayedHeadword = wordFolded;
    }
    else {
      displayedHeadword = tildeValue;
    }
  }

  articleText.clear();
  if (pos != articleData.size()) {
    articleText = uc_string(articleData, pos);
  }
}

uc_string DslDictionary::getArticle(const uc_string& word) {
  uc_string articleRawText;
  
  vector<uint32_t> articleOffsets = indexTree.findArticles(word);
  uc_string wordCaseFolded = Folding::applySimpleCaseOnly(word);
  
  unsigned len = articleOffsets.size();
  for (unsigned x = 0; x < len; ++x) {
    uc_string tildeValue;
    uc_string displayedHeadword;
    uc_string articleText;
    unsigned headwordIndex;

    try {
      loadArticle(articleOffsets[x], wordCaseFolded, tildeValue, displayedHeadword, headwordIndex, articleText);
      DslUtils::expandTildes(articleText, tildeValue);
      DslUtils::processSubentries(articleText);
      articleRawText += articleText;
    }
    catch (std::exception &ex) {
      Log::e("DSL: Failed loading article from \"%s\", reason: %s\n", getName().c_str(), ex.what());
    }
  }
  
  return articleRawText;
}


template<typename T> void DslDictionary::addWordsToIndex(const T& words, uint32_t offset) {
  for (auto word: words) {
    DslUtils::unescapeDsl(word);
    DslUtils::normalizeHeadword(word);
    indexTree.addWord(word, offset);
    
    if (Config::TEST_MODE) {
      allWords.push_back(word);
    }
  }
}
template void DslDictionary::addWordsToIndex<list<uc_string>>(const list<uc_string>& words, uint32_t offset);
template void DslDictionary::addWordsToIndex<vector<uc_string>>(const vector<uc_string>& words, uint32_t offset);


void DslDictionary::addSubEntry(vector<SubEntry>& subEntries, size_t subentryOffset, size_t lineOffset, vector<uc_string>& subHeadwords) {
  SubEntry subCard(subentryOffset, lineOffset - subentryOffset, subHeadwords);
  subEntries.push_back(subCard);
  subHeadwords.clear();
}

void DslDictionary::dumpIndexedWords() {
  Log::d("--Words--");
  vector<string> words = indexTree.getWords();
  for (auto w: words) {
    Log::d(w);
  }
}


uc_string DslDictionary::getAbbrevHtml() {
  Log::d("getAbbrevHtml()");
  
  uc_string result;
  size_t dslLine = 0;
  
  try {
    DslReader dslReader(dictPath);
    dslReader.readHeader();
    if (!dslReader.isDslFile()) throw runtime_error(Vars::ERROR_INCORRECT_DSL_FILE);
    dslReader.skipEmptyLines();
    
    uc_string dictName = dslReader.getDictionaryName();
    
    uc_string lineStr;
    size_t lineOffset;
    
    uint32_t articleCount = 0;
    uint32_t wordCount = 0;
    
    bool hasString = true;
    bool bodyEntered = false;
    
    while (hasString) {
      hasString = dslReader.readLine(lineStr, lineOffset);
      dslLine = dslReader.getLinesRead();
      
      bool isHeadword = lineStr.size() && !Utils::isWhitespace(lineStr[0]);
      bool isMainHeadword = isHeadword && (wordCount == 0 || bodyEntered);
      
      if (Utils::isEmpty(lineStr)) continue;
      
      if (bodyEntered) {
        result += U"</td></tr>";
      }
      bodyEntered = false;
      
      DslUtils::unescapeDsl(lineStr);
      
      if (isHeadword) {
        if (isMainHeadword) {
          result += U"<tr><td>";
          articleCount++;
        }
        else {
          result += U", ";
        }
        result += lineStr;
        
        wordCount++;
      }
      else {
        bodyEntered = true;
        result += U"</td>";
        result += U"<td>";
        result += lineStr;
      }
    }
    
    if (!bodyEntered) {
      Log::w("[WARNING]: Last headword has no body");
    }
    
    result += U"</tr>";
    result = U"<table class='abbrev-table'>" + result + U"</table>";
    result = U"<div class='abbrev-dict-name'>" + dictName + U"</div>" + result;
  }
  catch (std::exception & e) {
    Log::e("[ERROR]: DSL file reading: %s:%u. Details: %s\n", dictPath.c_str(), dslLine, e.what());
    return uc_string();
  }
  
  return result;
}
