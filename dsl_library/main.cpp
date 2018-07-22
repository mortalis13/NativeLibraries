
#include <iostream>
#include <fstream>
#include <string>

#include "src/vars.h"
#include "src/dpe.h"
#include "src/dsl_reader.h"
#include "src/utils.h"

#include "tests.h"

using namespace std;


enum Mode {
  TEST_MODE               = 1L << 0,
  GLOBAL_TEST_MODE        = 1L << 1,
  GLOBAL_TEST_INDEX_MODE  = 1L << 2,
  INDEX_MODE              = 1L << 3,
  SEARCH_MODE             = 1L << 4
};

// int mode = GLOBAL_TEST_MODE;
// int mode = INDEX_MODE;
// int mode = SEARCH_MODE;
int mode = TEST_MODE;

// int mode = INDEX_MODE | SEARCH_MODE;
// int mode = GLOBAL_TEST_MODE;


#define INDEX_DIR "e:/Data/HomeDict/index/"

// #define TEST_DSL "e:/Data/HomeDict/_error_test.dsl"
// #define TEST_DSL "e:/Data/HomeDict/_tests.dsl"
// #define TEST_DSL "e:/Data/HomeDict/not_exists.dsl"

// #define TEST_DSL "e:/Data/HomeDict/es-en.dsl"
// #define TEST_DSL "e:/Data/HomeDict/es-en_utf.dsl"
// #define TEST_DSL "e:/Data/HomeDict/es-en.dsl.dz"
// #define TEST_DSL "e:/Data/HomeDict/es-en_utf.dsl.dz"

// #define TEST_DSL "e:/Data/HomeDict/UniversalEsRu.dsl"
// #define TEST_DSL "e:/Data/HomeDict/UniversalEsRu.dsl.dz"

// #define TEST_DSL "e:/Data/HomeDict/ModernUsageEsRu.dsl"

// #define TEST_DSL "e:/Data/HomeDict/ru-es.dsl"
// #define TEST_DSL "e:/Data/HomeDict/ru-es.dsl.dz"

// #define TEST_DSL "e:/Data/HomeDict/es-en_2.dsl"
// #define TEST_DSL "e:/Data/HomeDict/es-en.dsl.gz"

// #define TEST_DSL "e:/Data/HomeDict/UniversalDeRu.dsl"
// #define TEST_DSL "e:/Data/HomeDict/UniversalDeRu.dsl.dz"
// #define TEST_DSL "e:/Data/HomeDict/UniversalDeRu_utf.dsl"
// #define TEST_DSL "e:/Data/HomeDict/UniversalDeRu_utf.dsl.dz"

// #define TEST_DSL "e:/Data/HomeDict/ru-es_wikidict.dsl"
// #define TEST_DSL "e:/Data/HomeDict/ru-es_wikidict.dsl.dz"
// #define TEST_DSL "e:/Data/HomeDict/ru-es_wikidict-2.dsl"

// #define TEST_DSL "e:/Data/HomeDict/_dsl/en-ru/sys/ComputersEnRu.dsl"
// #define TEST_DSL "e:/Data/HomeDict/_dsl/en-ru/sys/PhysicsEnRu.dsl"
// #define TEST_DSL "e:/Data/HomeDict/_dsl/ru-ru/sys/DahlRuRu.dsl"
// #define TEST_DSL "e:/Data/HomeDict/_dsl/en-en/sys/CollinsEnEn.dsl"
// #define TEST_DSL "e:/Data/HomeDict/_dsl/en-es/en-es_Babylon_English-Spanish_we_1_02.dsl"
// #define TEST_DSL "e:/Data/HomeDict/_dsl/ru-en/ru-en_for_translators.dsl"
// #define TEST_DSL "e:/Data/HomeDict/_dsl/es-es/es-es_DiccionarioRealAcademia-2.dsl"
// #define TEST_DSL "e:/Data/HomeDict/_dsl/es-es/es-es_abreviaturas.dsl"
#define TEST_DSL "e:/Data/HomeDict/_dsl/en-es/sys/OxfordConciseEnEs.dsl"


// #define TEST_WORD U"mesa"
// #define TEST_WORD U"l"
// #define TEST_WORD U"egyptian chars"
// #define TEST_WORD U"balde"
// #define TEST_WORD U"arbol"
// #define TEST_WORD U"acacia"
// #define TEST_WORD U"Acacia"
// #define TEST_WORD U"m"
// #define TEST_WORD U"ajuste"
// #define TEST_WORD U"nuevos"
// #define TEST_WORD U"información"
// #define TEST_WORD U"submesa"
// #define TEST_WORD U"datos"
// #define TEST_WORD U"seguridad"
// #define TEST_WORD U"me"
// #define TEST_WORD U"áplicacion"
// #define TEST_WORD U"aplicacione"
// #define TEST_WORD U"constitución"
// #define TEST_WORD U"deporte"
// #define TEST_WORD U"deport"
// #define TEST_WORD U"Acacia"
// #define TEST_WORD U"acacia"
// #define TEST_WORD U"sport"
// #define TEST_WORD U"kalt"
// #define TEST_WORD U"arbeiten"
// #define TEST_WORD U"батарея"
// #define TEST_WORD U"Creta"
// #define TEST_WORD U"Бет"
// #define TEST_WORD U"ПРОЦЕДУРНАЯ ТЕРМИНОЛОГИЯ"
// #define TEST_WORD U"вновь заявляет"
// #define TEST_WORD U"комиссия"
// #define TEST_WORD U"съезд"
// #define TEST_WORD U"info"
// #define TEST_WORD U"учредить комитет"
// #define TEST_WORD U"создать комитет"
// #define TEST_WORD U"составить программу работы"
// #define TEST_WORD U"выработать программу работы"
// #define TEST_WORD U"крайний срок"
// #define TEST_WORD U"срок подачи"
// #define TEST_WORD U"собраться"
// #define TEST_WORD U"oip"
// #define TEST_WORD U"OIP"
#define TEST_WORD U"dead"


string articleWord = "";

// -------------------------------------------------------------------

void indexDict(string dictPath) {
  Log::dg("-- indexDict()");
  
  string indexPath = Utils::getIndexPath(dictPath, INDEX_DIR);
  remove(indexPath.c_str());
  
  DPE::DictInfo dictInfo = DPE::indexDict(dictPath, indexPath);
  Log::d("Indexing result: " + dictInfo.direction);
}

void indexDictsBatch() {
  Log::d("-- indexDictsBatch()");
  
  vector<string> dicts = {
    "e:/Data/HomeDict/_dsl/de-en-de/sys/OxfordDudenConciseDeEn.dsl",
    "e:/Data/HomeDict/_dsl/de-en-de/sys/OxfordDudenConciseEnDe.dsl",
    "e:/Data/HomeDict/_dsl/de-es-de/sys/CompactVerlagDeEs.dsl",
    "e:/Data/HomeDict/_dsl/de-es-de/sys/CompactVerlagEsDe.dsl",
    "e:/Data/HomeDict/_dsl/de-ru-de/sys/UniversalDeRu.dsl",
    "e:/Data/HomeDict/_dsl/de-ru-de/sys/UniversalRuDe.dsl",
    "e:/Data/HomeDict/_dsl/en-en/sys/CollinsEnEn.dsl",
    "e:/Data/HomeDict/_dsl/en-es/sys/UniversalEnEs.dsl",
    "e:/Data/HomeDict/_dsl/en-ru/sys/LingvoUniversalEnRu.dsl",
    "e:/Data/HomeDict/_dsl/es-en/sys/OxfordConciseEsEn.dsl",
    "e:/Data/HomeDict/_dsl/es-en/es-en_A_Spanish_English_Dictionary_Granada_University_Spain.dsl",
    "e:/Data/HomeDict/_dsl/es-es/es-es_drae_an_1_2.dsl",
    "e:/Data/HomeDict/_dsl/es-es/es-es_clave_sinonimos.dsl",
    "e:/Data/HomeDict/_dsl/es-ru/sys/UniversalEsRu.dsl",
    "e:/Data/HomeDict/_dsl/ru-en/sys/UniversalRuEn.dsl",
    "e:/Data/HomeDict/_dsl/ru-es/sys/UniversalRuEs.dsl",
    "e:/Data/HomeDict/_dsl/ru-ru/sys/DahlRuRu.dsl"
  };
  
  for (auto dict: dicts) {
    string indexPath = Utils::getIndexPath(dict, INDEX_DIR);
    DPE::DictInfo dictInfo = DPE::indexDict(dict, indexPath);
  }
}

void searchWord() {
  Log::d("-- searchWord()");
  
  string indexPath = Utils::getIndexPath(TEST_DSL, INDEX_DIR);
  vector<string> res = DPE::searchWord(TEST_DSL, indexPath, TEST_WORD, 10);
  
  if (res.size() > 0) {
    articleWord = res[0];
  }
  
  fstream file("words.txt", ios_base::out | ios_base::binary);
  cout << "\n";
  for (auto item: res) {
    cout << item << "\n";
    Utils::writeToFile(file, item);
    Utils::writeToFile(file, "\n");
  }
  cout << "\n";
  file.close();
}

void showArticle() {
  Log::d("-- showArticle()");
  
  uc_string word = Utils::convert(articleWord);
  // string word = TEST_WORD;
  
  string indexPath = Utils::getIndexPath(TEST_DSL, INDEX_DIR);
  uc_string uc_articleText = DPE::getArticle(TEST_DSL, indexPath, word);
  string articleText = Utils::convert(uc_articleText);
  
  fstream file("article.txt", ios_base::out | ios_base::binary);
  Utils::writeToFile(file, articleText);
  file.close();
  
  if (articleText.empty()) Log::d("-- Article not found");
  
  // Utils::dump_bytes(articleText);
  
  cout << "\n'''-\n";
  cout << articleText << "\n-'''\n";
}


// ---------------------------- TESTS ----------------------------

void searchAllWords() {
  Log::dg("searchAllWords(), " + string(TEST_DSL));
  
  vector<string> words;
  
  fstream wordsFile("dict_words_dump_.txt", ios_base::in);
  string line;
  while (getline(wordsFile, line)) {
    words.push_back(line);
  }
  wordsFile.close();

  int found = 0;
  for (auto word: words) {
    string indexPath = Utils::getIndexPath(TEST_DSL, INDEX_DIR);
    vector<string> res = DPE::searchWord(TEST_DSL, indexPath, Utils::convert(word));
    
    if (res.size() > 0) {
      found++;
    }
    else {
      Log::dg("Word not found: " + word);
    }
  }
  
  Log::dg("-- Words found %d/%d", found, words.size());
}


void searchAllArticles() {
  Log::dg("searchAllArticles()");
  
  int count = 1;
  string dictPath;
  
  fstream dictsFile("dicts_list.txt", ios_base::in);
  while (getline(dictsFile, dictPath)) {
    Log::dg("[" + Utils::toStr(count) + "]: " + dictPath);
    if (dictPath.empty() || !Utils::fileExists(dictPath)) {
      count++;
      continue;
    }
    
    indexDict(dictPath);
    
    string dictFileName = Utils::toStr(count);
    string outFilePath = "dump/dict_dump_" + dictFileName + ".txt";
    fstream outFile(outFilePath, ios_base::out | ios_base::binary);
    
    Log::dg("Reading dict...");
    
    string word;
    int found = 0;
    int total = 0;
    
    fstream wordsFile("dict_words_dump_.txt", ios_base::in);
    while (getline(wordsFile, word)) {
      // Log::dg("Article for: " + word);
      
      string indexPath = Utils::getIndexPath(dictPath, INDEX_DIR);
      uc_string uc_articleText = DPE::getArticle(dictPath, indexPath, Utils::convert(word));
      string articleText = Utils::convert(uc_articleText);
      
      Utils::lineIndent(articleText);
      
      if (!articleText.empty()) {
        Utils::writeToFile(outFile, word + "\n  " + articleText + "\n");
        articleText.clear();
        found++;
      }
      else {
        Log::dg("Word not found: " + word);
      }
      
      total++;
    }
    
    Log::dg("-- Articles found %d/%d\n", found, total);
    
    wordsFile.close();
    outFile.close();
    
    // break;
    if (++count > 50) break;
  }
  
  dictsFile.close();
}

// -------------------------------------------------------------------

void localTest() {
  // int x1[] = {1,2,3,4,5};
  // std::find(std::begin(x1), std::end(x1), 3);
  
  // char buf[] {0x00, 0x00, 0x00, 0x61, 0x00, 0x00, 0x00, 0x62, 0x00, 0x00, 0x01, 0x80};
  // char buf[] {0x61, 0x00, 0x00, 0x00, 0x62, 0x00, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00};
  // uc_string s((uc_char*) buf, 3);
  // Log::d("s.size(): %d", s.size());
  // Log::d(s);
  
  uc_string str = U"123у≃";
  Utils::dump_bytes(str);
}


int main() {
  cout << "dsllib-1.2.1" << "\n\n";
  
  // localTest();
  // return 0;
  
  if (mode & INDEX_MODE) {
    indexDict(TEST_DSL);
  }
  
  if (mode & SEARCH_MODE) {
    searchWord();
    showArticle();
  }
  
  
  if (mode & TEST_MODE) {
    // test();
    testDslFile();
    // testDslBatch();
    // test14();
    // test16();
    // test3();
    // test11();
  }
  
  if (mode & GLOBAL_TEST_MODE) {
    Config::TEST_MODE = true;
    // searchAllWords();
    searchAllArticles();
  }

  return 0;
}
