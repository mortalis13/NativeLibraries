#include "basedictionary.h"

#include "utils.h"

void BaseDictionary::prefixMatch(const uc_string& str, vector<string>& mainMatches, vector<string>& fuzzyMatches, uint32_t maxResults) {
  try {
    indexTree.prefixMatch(str, mainMatches, fuzzyMatches, maxResults);
  }
  catch(std::exception& e) {
    Log::e("[ERROR]: Index searching failed: \"%s\", error: %s\n", getName().c_str(), e.what());
  }
}
