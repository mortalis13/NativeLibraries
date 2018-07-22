#include "dpe.h"

#include <algorithm>

#include "dsl_dictionary.h"
#include "dsl_parser.h"


namespace DPE {

DictInfo indexDict(const string& dictPath, const string& indexPath) {
  Config::STOP_INDEXING = false;
  
  DictInfo result;
  string errorMsg;
  
  if (dictPath.empty() || indexPath.empty()) {
    errorMsg = "[ERROR]: dictPath or indexPath empty, " + dictPath + "; " + indexPath;
    Log::e(errorMsg);
    result.errorMsg = errorMsg;
    return result;
  }
  
  if (!Utils::fileExists(dictPath)) {
    errorMsg = "[ERROR]: dictionary file does not exist, " + dictPath;
    Log::e(errorMsg);
    result.errorMsg = errorMsg;
    return result;
  }
  
  DslDictionary* dict = new DslDictionary(dictPath, indexPath);
  bool indexResult = dict->indexDict(errorMsg);
  
  if (indexResult) {
    string dictName = dict->getName();
    string direction = dict->getDirection();
    uint32_t wordCount = dict->getWordCount();
    uint32_t articleCount = dict->getArticleCount();
    
    result.name = dictName;
    result.direction = direction;
    result.headwordsCount = wordCount;
    result.articlesCount = articleCount;
    result.indexError = false;
    
    if (Config::TEST_MODE) {
      vector<uc_string> words = dict->getAllWords();
      // fstream file("dict_words_dump_" + dictName + ".txt", ios_base::out | ios_base::binary);
      fstream file("dict_words_dump_.txt", ios_base::out | ios_base::binary);
      for (auto word: words) {
        string word_utf;
        Utils::convert(word_utf, word);
        Utils::writeToFile(file, word_utf + "\n");
      }
      file.close();
    }
    
    Log::d("Dictionary indexed: " + dictName + ", " + direction + ", " + dictPath);
    Log::d("articleCount: %d, wordCount: %d\n", articleCount, wordCount);
  }
  else {
    result.indexError = true;
    result.errorMsg = errorMsg;
  }
  
  delete dict;
  
  return result;
}


DictInfo scanAbbrevDict(const string& dictPath) {
  DictInfo result;
  string errorMsg;
  
  if (dictPath.empty()) {
    errorMsg = "[ERROR]: dictPath empty, " + dictPath;
    Log::e(errorMsg);
    result.errorMsg = errorMsg;
    return result;
  }
  
  if (!Utils::fileExists(dictPath)) {
    errorMsg = "[ERROR]: dictionary file does not exist, " + dictPath;
    Log::e(errorMsg);
    result.errorMsg = errorMsg;
    return result;
  }
  
  DslDictionary* dict = new DslDictionary(dictPath);
  bool indexResult = dict->scanAbbrevDict(errorMsg);
  
  if (indexResult) {
    string dictName = dict->getName();
    string direction = dict->getDirection();
    uint32_t wordCount = dict->getWordCount();
    uint32_t articleCount = dict->getArticleCount();
    
    result.name = dictName;
    result.direction = direction;
    result.headwordsCount = wordCount;
    result.articlesCount = articleCount;
    result.indexError = false;
    
    Log::d("Abbrev Dictionary scanned: " + dictName + ", " + direction + ", " + dictPath);
  }
  else {
    result.indexError = true;
    result.errorMsg = errorMsg;
  }
  
  delete dict;
  
  return result;
}

vector<string> searchWord(const string& dictPath, const string& indexFile, const uc_string& word, uint32_t maxResults) {
  Log::d(U"searchWord(): " + word);
  Config::STOP_SEARCH = false;
  
  vector<string> result;
  
  if (dictPath.empty() || indexFile.empty()) {
    Log::e("[ERROR]: dictPath or indexFile empty, " + dictPath + "; " + indexFile);
    return result;
  }
  
  if (!Utils::fileExists(dictPath) || !Utils::fileExists(indexFile)) {
    Log::e("[ERROR]: dictionary or index file does not exist, " + dictPath + "; " + indexFile);
    return result;
  }
  
  uc_string wordNorm = word;
  Utils::trim(wordNorm);
  if (wordNorm.empty()) return result;
  
  vector<string> mainMatches, fuzzyMatches;
  DslDictionary dict(dictPath, indexFile);
  dict.readIndex();
  dict.prefixMatch(wordNorm, mainMatches, fuzzyMatches, maxResults);
  
  Log::d("Sorting results");
  std::sort(mainMatches.begin(), mainMatches.end(), Utils::cmpWords);
  for (auto w: mainMatches) {
    if (maxResults != 0 && result.size() >= maxResults) break;
    result.push_back(w);
  }
  
  if (maxResults == 0 || result.size() < maxResults) {
    std::sort(fuzzyMatches.begin(), fuzzyMatches.end(), Utils::cmpWords);
    for (auto w: fuzzyMatches) {
      if (maxResults != 0 && result.size() >= maxResults) break;
      result.push_back(w);
    }
  }
  
  return result;
}

vector<string> searchWord(const vector<string>& dictFiles, const string& indexPath, const uc_string& word, uint32_t maxResults) {
  Log::d(U"searchWord(): " + word);
  Config::STOP_SEARCH = false;
  
  vector<string> result;
  
  if (dictFiles.size() == 0) return result;
  
  uc_string wordNorm = word;
  Utils::trim(wordNorm);
  if (wordNorm.empty()) return result;
  
  vector<string> mainMatches, fuzzyMatches;
  for (auto dictPath: dictFiles) {
    Log::d("Searching in '" + dictPath + "'");
    if (Config::STOP_SEARCH) return vector<string>();
    
    if (dictPath.empty()) {
      Log::e("[ERROR]: dictPath empty");
      continue;
    }
    
    if (indexPath.empty()) {
      Log::e("[ERROR]: indexPath empty");
      return result;
    }
    
    string indexFile = Utils::getIndexPath(dictPath, indexPath);
    
    if (!Utils::fileExists(dictPath) || !Utils::fileExists(indexFile)) {
      Log::e("[ERROR]: dictionary or index file does not exist, " + dictPath + "; " + indexFile);
      continue;
    }
    
    DslDictionary dict(dictPath, indexFile);
    dict.readIndex();
    dict.prefixMatch(wordNorm, mainMatches, fuzzyMatches, maxResults);
  }
  
  Log::d("Sorting results");
  std::sort(mainMatches.begin(), mainMatches.end(), Utils::cmpWords);
  
  for (auto w: mainMatches) {
    if (maxResults != 0 && result.size() >= maxResults) break;
    result.push_back(w);
  }
  
  if (maxResults == 0 || result.size() < maxResults) {
    std::sort(fuzzyMatches.begin(), fuzzyMatches.end(), Utils::cmpWords);
    for (auto w: fuzzyMatches) {
      if (maxResults != 0 && result.size() >= maxResults) break;
      result.push_back(w);
    }
  }
  
  // Log::d("--WORDS--");
  // for (auto w: mainMatches) {
  //   Log::d(w);
  // }
  
  return result;
}

uc_string getArticle(const string& dictPath, const string& indexFile, const uc_string& word) {
  Log::d("DPE::getArticle()");
  
  uc_string articleContentHtml;
  
  if (dictPath.empty() || indexFile.empty()) {
    Log::e("[ERROR]: dictPath or indexFile empty, " + dictPath + "; " + indexFile);
    return uc_string();
  }
  
  if (!Utils::fileExists(dictPath) || !Utils::fileExists(indexFile)) {
    Log::e("[ERROR]: dictionary or index file does not exist, " + dictPath + "; " + indexFile);
    return uc_string();
  }
  
  uc_string wordNorm = word;
  Utils::trim(wordNorm);
  if (wordNorm.empty()) return uc_string();
  
  DslDictionary dict(dictPath, indexFile);
  dict.readIndex();
  uc_string articleContent = dict.getArticle(wordNorm);
  
  bool generateHtml = true;
  // bool generateHtml = false;
  
  if (generateHtml) {
    Utils::lineTrim(articleContent);
    string dictName = dict.getName();
    
    DslParser dslParser(articleContent, dictName, Utils::convert(wordNorm));
    articleContentHtml = dslParser.generateHtml();
    
    if (articleContentHtml.empty()) articleContentHtml = articleContent;
  }
  else {
    articleContentHtml = articleContent;
  }
  
  return articleContentHtml;
}


uc_string getAbbrevDictHtml(const string& dictPath) {
  Log::d(U"getAbbrevsHtml()");
  
  uc_string result;
  
  if (dictPath.empty()) {
    Log::e("[ERROR]: dictPath empty");
    return uc_string();
  }
  
  if (!Utils::fileExists(dictPath)) {
    Log::e("[ERROR]: dictionary does not exist, " + dictPath);
    return uc_string();
  }
  
  DslDictionary dict(dictPath);
  result = dict.getAbbrevHtml();
  
  return result;
}


void stopIndexing() {
  Config::STOP_INDEXING = true;
}

void stopSearch() {
  Config::STOP_SEARCH = true;
}

}
