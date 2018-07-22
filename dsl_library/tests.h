
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

#include <iconv.h>

#include "src/folding.h"
#include "src/dsl_utils.h"
#include "src/article_dom.h"
#include "src/dsl_parser.h"


void test() {
  string s = "abc ≃";
  u32string s32 = U"abc ≃";
  
  Log::d("s.size(): %d", s.size());
  Log::d("s32.size(): %d", s32.size());
  
  cout << s << "\n";
  Log::d(s32);
  
  s32 += U"<span class='fx'>≃</span>";
  Log::d(s32);
}


string getTagName(const string& tag) {
  string tagName;

  string fullTag = tag;
  fullTag.erase(fullTag.find_first_of('['), 1);
  fullTag.erase(fullTag.find_last_of(']'), 1);
  Utils::trim(fullTag);
  
  size_t len = fullTag.size();
  for (size_t i = 0; i < len; i++) {
    if (fullTag[i] == '/') continue;
    if (fullTag[i] == ' ') break;
    tagName += fullTag[i];
  }
  
  return tagName;
}


void getTagData(const string& tag, string& tagName, string& tagAttrs) {
  string tagNorm = tag;
  if (tag.find('[') != npos) tagNorm.erase(tagNorm.find_first_of('['), 1);
  if (tag.find(']') != npos) tagNorm.erase(tagNorm.find_last_of(']'), 1);
  Utils::trim(tagNorm);
  
  bool attrStarted = false;
  
  size_t len = tagNorm.size();
  for (size_t pos = 0; pos < len; pos++) {
    if (tagNorm[pos] == '/') continue;
    
    if (attrStarted) {
      tagAttrs += tagNorm[pos];
    }
    else if (tagNorm[pos] == ' ') {
      attrStarted = true;
      size_t attrPos = tagNorm.find_first_not_of(' ', pos);
      if (attrPos != npos) pos = attrPos - 1;
    }
    else {
      tagName += tagNorm[pos];
    }
  }
}

void getTagData3(const string& tag, string& tagName, string& tagAttrs) {
  string tagNorm = tag;
  tagName = tag;
  // if (tag.find("[") != npos) tagNorm.erase(tagNorm.find_first_of("["), 1);
  // if (tag.find("]") != npos) tagNorm.erase(tagNorm.find_last_of("]"), 1);
  // Utils::trim(tagNorm);
  
  size_t tagNameStart = tagNorm.find_first_not_of("[ /");
  size_t tagNameEnd = tagNorm.find_first_of(" ]", tagNameStart);
  
  size_t tagAttrsStart = tagNorm.find_first_not_of(" ]", tagNameEnd);
  size_t tagAttrsEnd = tagNorm.find_first_of("]", tagAttrsStart);
  
  // Log::d("%s:\t\t[%d:%d],\t\t[%d:%d]", tagNorm.c_str(), tagNameStart, tagNameEnd, tagAttrsStart, tagAttrsEnd);
  
  if (tagNameStart != npos)
    tagName = tagNorm.substr(tagNameStart, tagNameEnd - tagNameStart);
  
  if (tagAttrsStart != npos)
    tagAttrs = tagNorm.substr(tagAttrsStart, tagAttrsEnd - tagAttrsStart);
  
  if (!tagName.empty() && (tagName[0] == ' ' || tagName[tagName.size()-1] == ' ')) Utils::trim(tagName);
  if (!tagAttrs.empty() && (tagAttrs[0] == ' ' || tagAttrs[tagAttrs.size()-1] == ' ')) Utils::trim(tagAttrs);
}


void test18() {
  // string s = "abc";
  // // s=s.substr(1, -1);
  // s=s.substr(1, -10);
  // // s=s.substr(-1, 2);
  // cout << s;
  // return;
  
  vector<string> ss = {
    "[ c blue]",
    "[*]",
    "[/*]",
    "[/b]",
    "[/c]",
    "[/i]",
    "[/lang]",
    "[/m]",
    "[/p]",
    "[/sup]",
    "[/u]",
    "[b]",
    "[c blue]",
    "[c crimson]",
    "[c darkblue]",
    "[c darkorange]",
    "[c gray]",
    "[i]",
    "[lang id=1033]",
    "[m0]",
    "[m1]",
    "[m2]",
    "[m3]",
    "[m4]",
    "[m5]",
    "[m]",
    "[p]",
    "[sup]",
    "[u]",
    "[  abc     pre=5  to=123    ]",
    "m4",
    "[abc",
    "qwe]",
    "abc 3213 pqowek",
    "      ",
    " popoqpo   ",
  };
  
  for (auto tag: ss) {
    string tagName;
    string tagAttrs;
    getTagData(tag, tagName, tagAttrs);
    Log::d(tag + ": '" + tagName + "', '" + tagAttrs + "'");
    
    // string tagName = getTagName(fullTag);
    // Log::d(tagName);
    
    // fullTag.erase(fullTag.find_first_of('['), 1);
    // fullTag.erase(fullTag.find_last_of(']'), 1);
    // Utils::trim(fullTag);
    // Log::d(fullTag);
  }
}

void test17() {
  // string s = "abc";
  // cout << *s.begin() << "\n";
  // cout << *(s.end()-1) << "\n";
  
  uc_string s = U"abc";
  // cout << *s.begin() << "\n";
  // cout << *(s.end()-1) << "\n";
  cout << s[0] << "\n";
  cout << s[2] << "\n";
  cout << s[s.size()-1] << "\n";
}

void test16() {
  vector<string> ss = {
    "187238",
    "",
    "   ",
    "\t\t\t",
    "\n\n",
    "\t\n ",
    "\r\t\r\r\n    \t",
    "  abcd  \r\n\t",
    "   dqw     ",
    " qwe ",
    " 123",
    "234 ",
    "\t000",
    "   \ta la carta\r\n\r\n  ",
    "  information\r\n  \r\n  \r\n",
  };
  
  for (auto s: ss) {
    // Utils::trim(s);
    Utils::lineTrim(s);
    Log::d("''" + s + "''");
  }
}

void testDslBatch() {
  vector<string> dslTests = {
    "",
    "  \t\r\n  ",
    "[m1][p][i]n[/i][/p] [lang]-s[/lang]123[/m]",
    "[m1][p][i]n[/i][/p] [lang id=1031]-s[/lang]123",
    "[m1][p]123[i]n[/i][/p] [lang id=1031]-s[/lang][/m]",
    "bb[m1][p][i][c][com]n[/i][/p] [lang id=1031]-s[/lang][/com][/c][/m]",
    "[m1][p][i][c][com]n[/i][/p] [lang id=1031]-s[/lang][/com][/c]",
    "[m1][p][i]n[/i][/p] [lang id=1031]-s[/lang][/m][m2]123[/m]",
    "[m1][p][i]n[/i][/p] [lang id=1031]-s[/lang][/m][m2]123",
    "[m2]1. [i][b][c darkorange]prep[/c][/b][/i] [c darkcyan]localización[/c][/m]",
    
    "[m1][p][i]n[/i][/p] -s[/a]123[/m]",
    "[m1][p][i]n[/i][/a][/p] -s[/m]",
    "[m1][p][i][c][com]n[/i][/p] [lang id=1031]-s[/lang][/m]",
    "[m1][p][i][c][com]n[/i][/p] [lang id=1031]-s[/m]",
    "[m1][p][i][c][com]n[/i][/p] [lang id=1031][b]-s[/b][/m]",
    "[m1][p][i][c][com]n[/i][/p] [lang id=1031]-s[/m][m2]123[/m]",
    "[m1][p][i][c][com]n[/i][/p] [lang id=1031]-s[/m][m2]123[/m][m3]qwe[m2]000[/m]",
    "[m1][p][i]n[/i] [lang id=1031]-s[/lang][/m]",
    "[m1][i][com]n[/i] [b][lang id=1031]-s[/b][/m]",
    "[m1][i]123[com]n[/i] [b][lang id=1031]-s[/b][/m]",
    
    "[m1][c][trn][com]¡[lang id=1034]agua \\[va\\]![/lang][/com][/c] берег[']и[/']сь!; посторон[']и[/']сь![/trn][/m]",
    "[m1][trn]\\[[i][com]удивление[/com][/i]\\] ничег[']о[/'] себ[']е[/']!; хор[']о[/']шенькое д[']е[/']ло![/trn][/m]",
    "[*][m3]<<accionador de membrana>>[/m][/*]",
    "  [*][m2][b]Синонимы:[/b] <<acción derivada>>, <<acción por derivación>>, <<acción derivación>>[/m][/*]",
    
    "  [m1][p]m[/p][/m]\r\n  [m1]1) [p][trn]книжн[/p] торг[']о[/']вый г[']о[/']род, центр[/trn][/m]",
    "  [m1][p]m[/p]\r\n  [m2]1) [p][trn]книжн[/p] торг[']о[/']вый г[']о[/']род, центр[/trn][/m]",
    "  [m1][p]m[/p]123\n  [m2]1) [p][trn]книжн[/p] торг[']о[/']вый г[']о[/']род, центр[/trn][/m]\n",
    "  персонал, личный состав, штат, кадры",
    "[m1]1) [trn]толкн[']у[/']ть; пихн[']у[/']ть [p]разг[/trn][/p][/m]",
    "  [m2]- [ref]de balde[/ref]\n  - [ref]estar de balde[/ref][/m]",
    "  \\[[t]'iːˌbeɪə[/t]\\]\n  [m1][i][c][com]or[/i] [b]ebayer[/b][/m]\n  [m1][p][i]n[/com][/c][/i][/p][/m]",
    " [m1][i]см[/i] <<meteoro>>[m]",
    "  [m1]Proverbios y refranes españoles par Justo Fernández López (Es-De)\n  [m2]Ver. 1.0 (red. 08.03.2014)\n  [m3]Number of headwords: 4150.",
    "  [m2]♦ Gehe nie zu deinem Fürst, wenn du nicht gerufen bist.\r\n",
    "[m1][p]adj[/p][/m], [p]adv[/p] a la carta[/m]",
  };
  
  for (auto s: dslTests) {
    Log::d("------source------");
    Log::d(s);
    Log::d("------------------\n");
    
    Utils::lineTrim(s);
    Utils::trim(s);
    
    // DslParser dslParser(s, "__", "__");
    // string res = dslParser.generateHtml(true);
    // Log::d("------result------");
    // Log::d(res);
    // Log::d("==================\n\n");
    
    // ArticleDom dom(Utils::convert(s), "__", U"__");
    // dom.dumpDom();
    // Log::d("\n" + s);
    // Log::d("------------\n");
  }
}


void testDslFile() {
  string fp = "E:/Documents/8-proyectos/cpp/eclipse-oxygen-cpp/dsllib-1.0.3/data/test_article_to_html.txt";
  fstream f;
  f.open(fp, ios_base::in | ios_base::binary);
  
  f.seekg(0, f.end);
  size_t size = f.tellg();
  f.seekg(0);
  
  char buf[size];
  f.read(buf, size);
  f.close();
  
  string text(buf, size);
  uc_string articleText = Utils::convert(text);
  
  bool b = false;
  DslUtils::stripComments(articleText, b);
  
  Utils::lineTrim(articleText);
  // Utils::trim(articleText);
  
  DslParser dslParser(articleText, "TEST_DICT", "TEST_WORD");
  uc_string res = dslParser.generateHtml(true);
  
  Log::d(U"\n-----\n" + res + U"\n-----\n\n");
  
  // DslParser dslParser(articleText, "__", "__");
  // string res = dslParser.generateHtml(true);
  // Log::d("\n-----\n" + res + "\n-----\n\n");
  
  // ArticleDom dom(Utils::convert(articleText), "__", U"__");
  // dom.dumpDom();
}

void test15() {
  // uc_string s = U"[m1][p][i][c][com]n[/i][/p] [lang id=1031]-s[/lang][/com][/c][/m]\n";
  uc_string s = U"[m1][p][i][c][com]n[/i][/p] [lang id=1031]-s[/lang][/m]";
  Log::d(s);
  
  string dictName = "DN";
  uc_string headword_ = U"HW";
  
  ArticleDom dom(s, dictName, headword_);
  
  Log::d("Final, root: %d", dom.root.size());
  dom.dumpDom();
}

void test14() {
  string fp = "e:/Documents/8-proyectos/cpp/eclipse-oxygen-cpp/dsllib-1.0.3/data/test_article.txt";
  fstream f;
  f.open(fp, ios_base::in | ios_base::binary);
  
  f.seekg(0, f.end);
  size_t size = f.tellg();
  f.seekg(0);
  
  char buf[size];
  f.read(buf, size);
  f.close();
  
  uc_string articleText;
  Utils::convert(articleText, buf, size, ENC_UCS4LE, ENC_UTF16LE);
  
  Log::d(U"\n-----\n" + articleText + U"\n-----\n\n");
  DslUtils::processSubentries(articleText);
  // Utils::lineTrim(articleText);
  Log::d(U"\n-----\n" + articleText + U"\n-----\n\n");
  
  // DslParser dslParser(Utils::convert(articleText));
  // string res = dslParser.generateHtml();
  // Log::d("\n-----\n" + res + "\n-----\n\n");
}


bool cmpWords(const string &a, const string &b) {
  uc_string us1 = Folding::apply(Utils::convert(a));
  uc_string us2 = Folding::apply(Utils::convert(b));
  return us1 < us2;
}

struct {
  bool operator()(const string &a, const string &b) {
    uc_string us1 = Folding::apply(Utils::convert(a));
    uc_string us2 = Folding::apply(Utils::convert(b));
    return us1 < us2;
  }
} cmpWordsObj;

struct {
bool operator()(const string &a, const string &b) {
  // return a.compare(b);
  return a < b;
}
} cmpStd;

void test13() {
  vector<string> v1 {"arbolado", "arbolario", "arbolar", "arbolero", "árbol", "arboleda"};
  vector<string> v2 {"arbolado", "arbolario", "arbolar", "Arbolero", "árbol", "arboleda"};
  vector<string> v3 {"arbolado", "arbolario", "arbolar", "arbolero", "arbol", "arboleda"};

  sort(v1.begin(), v1.end());
  for (auto str: v1) cout << str << "\n";
  cout << "\n";

  sort(v2.begin(), v2.end(), cmpWords);
  for (auto str: v2) cout << str << "\n";
  cout << "\n";

  sort(v3.begin(), v3.end(), cmpStd);
  for (auto str: v3) cout << str << "\n";
  cout << "\n";
}

void test12() {
  char src[] = {0x61, 0x00, 0x62, 0x00, 0x63, 0x00, 0xa1, 0x00};
  uc_char outBuff[1];
  
  char *pIn = src;
  uc_char *pOut = outBuff;
  
  size_t inRemains = 2;
  size_t outRemains = 2;

  printf("inRemains: %d, outRemains: %d\n", inRemains, outRemains);

  iconv_t cb = iconv_open("UCS-2LE", "UTF-16LE");
  size_t cvtlen = iconv(cb, (char**) &pIn, &inRemains, (char**) &pOut, &outRemains);
  iconv_close(cb);
  
  printf("inRemains: %d, outRemains: %d, cvtlen: %d\n", inRemains, outRemains, cvtlen);
  
  if (cvtlen == (size_t) -1) {
    printf("error: %s, %d\n", strerror(errno), errno);
  }
  
  
  cout << "\n";
  for(int i = 0; i<8; i++) {
    printf("%02x ", (unsigned char) src[i]);
  }
  cout << "\n";
  
  printf("%04x", outBuff[0]);
  cout << "\n";
}

void test11() {
  // uc_string curString = u"\\(c\\)";
  // uc_string curString = u"¡oiga{(n)}!";
  // uc_string curString = u"¡oiga{(}n{)}!";
  // uc_string curString = u"a {(}buen{)} punto";
  // uc_string curString = u"ovni\\(s\\)";
  // uc_string curString = u"Abbröck(e)lung";
  // uc_string curString = U" abc    qwe dslko  fkpokef  ";
  // uc_string curString = U"batería {{com1";
  // uc_string curString = U"  {{comment in body}}";
  uc_string curString = U"          \t\t\t     \t  ";
  
  // DslUtils::processUnsortedParts(curString, true);
  // list<uc_string> allEntryWords;
  // DslUtils::expandOptionalParts(curString, &allEntryWords);
  // Log::d(curString);
  // Log::d("--Headings--");
  // for (auto s: allEntryWords) {
  //   Log::d(s);
  // }
  
  // DslUtils::normalizeHeadword(curString);
  // Log::d(curString);
  
  // bool hasComment = false;
  // DslUtils::stripComments(curString, hasComment);
  // Log::d(curString);
  // cout << hasComment;
  
  uc_string s = Folding::applyWhitespaceOnly(curString);
  Log::d(U"_" + s + U"_");
  cout << s.empty();
}

void test10() {
  #ifdef __WIN32
    cout << "win32";
  #else
    cout << "no-win32";
  #endif
}

void test9() {
  string w = "mesa";
  string w1 = "mésa";
  
  uc_string res1 = Utils::convert(w);
  Utils::dump_bytes(w);
  Utils::dump_bytes(res1);
  
  uc_string res2 = Utils::convert(w1);
  Utils::dump_bytes(w1);
  Utils::dump_bytes(res2);
}

void test8() {
  char src[] = {0x61, 0x00, 0x62, 0x00, 0x63, 0x00, 0xa1, 0x00};
  uc_char outBuff[1];
  
  char *pIn = src;
  uc_char *pOut = outBuff;
  
  size_t inRemains = 2;
  size_t outRemains = 2;

  printf("inRemains: %d, outRemains: %d\n", inRemains, outRemains);

  iconv_t cb = iconv_open("UCS-2LE", "UTF-16LE");
  size_t cvtlen = iconv(cb, (char**) &pIn, &inRemains, (char**) &pOut, &outRemains);
  iconv_close(cb);
  
  printf("inRemains: %d, outRemains: %d, cvtlen: %d\n", inRemains, outRemains, cvtlen);
  
  if (cvtlen == (size_t) -1) {
    printf("error: %s, %d\n", strerror(errno), errno);
  }
  
  
  cout << "\n";
  for(int i = 0; i<8; i++) {
    printf("%02x ", (unsigned char) src[i]);
  }
  cout << "\n";
  
  printf("%04x", outBuff[0]);
  cout << "\n";
}


void test7() {
  char str[] = "ab";
  int strl = strlen(str);
  cout << strl << "\n";
  
  const char* src = "ab";
  size_t len = 2;
  
  uc_char outBuff[len + 1];
  char* pout = (char*) outBuff;
  
  size_t inRemains = len;
  size_t outRemains = len * sizeof(uc_char);

  printf("inRemains:%d outRemains:%d\n", (int)inRemains, (int)outRemains);

  iconv_t cb = iconv_open("UTF-16LE", "UTF-8");
  size_t cvtlen = iconv(cb, (char**)&src, (size_t*)&inRemains, (char**)&pout, (size_t*)&outRemains);
  iconv_close(cb);
  
  if (cvtlen == (size_t) -1) {
    printf("error:%s, %d\n", strerror(errno), errno);
    return;
  }

  printf("inRemains:%d outRemains:%d cvtlen:%d\n", (int)inRemains, (int)outRemains, (int)cvtlen);

  for (size_t i = 0; (i < len) && outBuff[i] ; i++) {
    printf("0x%04x\n", outBuff[i]);
  }
}


void test6() {
  char str[] = "ab";
  int strl = strlen(str);
  cout << strl << "\n";
  
  const char* src = "ab";
  size_t len = 4;
  
  uint16_t* outBuff = new uint16_t[len + 1];
  char* pout = (char*) outBuff;
  
  size_t inRemains = len;
  size_t outRemains = (len + 1) * sizeof(uint16_t);

  printf("inRemains:%d outRemains:%d\n", (int)inRemains, (int)outRemains);

  iconv_t cb = iconv_open("UTF-16", "UTF-8");
  size_t cvtlen = iconv(cb, (char**)&src, (size_t*)&inRemains, (char**)&pout, (size_t*)&outRemains);
  iconv_close(cb);
  
  if (cvtlen == (size_t) -1) {
    printf("error:%s, %d\n", strerror(errno), errno);
    return;
  }

  printf("inRemains:%d outRemains:%d cvtlen:%d\n", (int)inRemains, (int)outRemains, (int)cvtlen);

  for (size_t i = 0; (i < len) && outBuff[i] ; i++) {
    printf("0x%04x\n", outBuff[i]);
  }
}

void test4() {
  char src[] = {0x61, 0x00, 0x62, 0x00};
  char dst[100];
  
  size_t srclen = 4;
  size_t dstlen = 2;
  
  char *pIn = src;
  char *pOut = dst;

  iconv_t conv = iconv_open("UCS-2LE", "UTF-16LE");
  // iconv_t conv = iconv_open("UTF-8", "UTF-16LE");
  iconv(conv, (char**) &pIn, &srclen, (char**) &pOut, &dstlen);
  iconv_close(conv);
  
  for(int i=0; i<4; i++) {
    cout << hex << (short) dst[i] << " ";
  }
}

void iconv1() {
  char src[] = "abcčde";
  char dst[100];
  size_t srclen = 6;
  size_t dstlen = 12;

  char * pIn = src;
  char * pOut = ( char*)dst;

  iconv_t conv = iconv_open("UTF-8", "CP1250");
  iconv(conv, &pIn, &srclen, &pOut, &dstlen);
  iconv_close(conv);
  
  cout << dst;
}

void createRawString() {
  char buf[10] = {0x61, 0x00, 0x62, 0x00};
  unsigned char b1, b2;
  uc_char c16;
  
  vector<uc_char> buf16;
  
  b1 = (unsigned char) buf[0];
  b2 = (unsigned char) buf[1];
  c16 = (b2 << 8) | b1;
  buf16.push_back(c16);
  
  b1 = (unsigned char) buf[2];
  b2 = (unsigned char) buf[3];
  c16 = (b2 << 8) | b1;
  buf16.push_back(c16);
  
  uc_string res(&buf16.front(), 2);
  
  Log::d(res);
}

void test3() {
  int atLine = 0;
  
  string dslPath = "e:/Programas/Dicts/GoldenDict/temp/es-en_utf.dsl";
  
  try {
    DslReader scanner(dslPath);
    scanner.readHeader();
    
    uc_string dictName = scanner.getDictionaryName();
    uc_string langFrom = scanner.getLangFrom();
    uc_string langTo = scanner.getLangTo();
    
    Log::d(dictName);
    Log::d(langFrom);
    Log::d(langTo);
  }
  catch (std::exception & e) {
    Log::d("DSL dictionary reading failed: %s:%u, error: %s\n", dslPath.c_str(), atLine+1, e.what());
  }
}

void test2() {
// void test() {
  // string str;
  // u32string s16 = U"123";
  
  // for (auto c = s16.begin(); c != s16.end(); ++c) {
  //   char t = *c;
  //   str.push_back(t);
  // }
  // cout << str;
  
  
  // string str;
  // uc_string s16 = u"123";
  
  // for (auto c = s16.begin(); c != s16.end(); ++c) {
  //   char t = *c;
  //   str.push_back(t);
  // }
  // cout << str;
  
  
  // char c1 = 0x12;
  // char c2 = 0x34;
  // uc_char c16 = (c1 << 8) | c2;
  // // uc_char c16 = (c1 << 8) + c2;
  // // uc_char c16 = c1 + c2;
  
  // cout << hex << c16;
  
  // int x = 0x12345678;
  // char *cp = (char*) &x;
  // // char *cp = reinterpret_cast<char*>(&x);
  
  // cout << "-" << hex << (short) *cp++;
  // cout << "-" << hex << (short) *cp++;
  // cout << "-" << hex << (short) *cp++;
  // cout << "-" << hex << (short) *cp++;
  
  
  // uc_string s16({0x6100, 0x6200, 0x6300});
  // wcout << s16;
  
  
  // uc_string s16({0x6100});
  // uc_char* p = &s16[0];
  // cout << hex << *p;
  
  // char buf[10] = {0x61, 0x00, 0x12, 0x62};
  // char *pc = buf;
  // uc_char *pc16 = (uc_char*) buf;
  
  // for (uc_char c: buf) {
  //   cout << hex << c << '\n';
  // }
  
  // for (char c: buf) {
  //   cout << hex << (short) c << '\n';
  // }
  
  // cout << sizeof(uc_char) << '\n';
  // cout << "-" << hex << *pc16++;
  // cout << '\n';
  // cout << "-" << hex << *pc16++;
  // cout << '\n';
  
  // cout << "-" << hex << (short) *pc++;
  // cout << '\n';
  // cout << "-" << hex << (short) *pc++;
  // cout << '\n';
  
  // uc_char c = 0x6100;
  // cout << hex << c;
}

void test1() {
  uc_string us;
  string s = "abc";
  s += "d";

  for (char c: s) {
    cout << c << " - 0x" << hex << (short) c << dec << '\n';
  }

  cout << s << '\n';
}
