#ifndef DSL_UTILS_H
#define DSL_UTILS_H

#include <string>
#include <list>
#include <stack>
#include <vector>

#include "vars.h"
#include "utils.h"

using namespace std;


namespace DslUtils {
  
  pos_t findSubentryChar(const uc_string& str);
  bool isSubentryClose(const uc_string& s);
  
  bool isHeaderStart(char b1, char b2);
  bool isHeadwordStart(char b1, char b2);
  
  bool isHeaderStart(char b1);
  bool isHeadwordStart(char b1);
  
  void processUnsortedParts(uc_string& str, bool strip);
  void expandOptionalParts(uc_string& str, list<uc_string>* result, size_t x = 0, bool inside_recurse = false);
  void expandTildes(uc_string& str, const uc_string& tildeReplacement);
  void unescapeDsl(uc_string& str);
  void normalizeHeadword(uc_string& str);
  void stripComments(uc_string& str, bool& nextLine);
  void stripComments(string& str, bool& nextLine);
  
  uc_string getLineLevelTag(const uc_string& str);
  void processSubentries(uc_string& str);

}

#endif