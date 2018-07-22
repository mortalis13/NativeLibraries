#ifndef DSL_PARSER_H
#define DSL_PARSER_H

#include <algorithm>
#include <string>
#include <list>
#include <stack>
#include <vector>

#include "vars.h"
#include "utils.h"

using namespace std;


class DslParser {
  
  const vector<uc_string> SKIP_TAGS = {
    U"'",
    U"*",
    U"com",
    U"trn",
    U"lang",
    U"!trs",
    U"t",
  };
  
  const vector<uc_string> REMOVE_TAGS = {
    U"s",
  };
  
  const vector<uc_string> DSL_TAGS = {
    U"!trs",
    U"'",
    U"*",
    U"b",
    U"c",
    U"com",
    U"ex",
    U"i",
    U"lang",
    U"m",
    U"m0",
    U"m1",
    U"m2",
    U"m3",
    U"m4",
    U"m5",
    U"m6",
    U"m7",
    U"m8",
    U"m9",
    U"p",
    U"ref",
    U"s",
    U"sub",
    U"sup",
    U"t",
    U"trn",
    U"u",
    U"url",
  };
  
  const uc_string HTML_TAG_START = U"<";
  const uc_string HTML_TAG_END = U">";
  const static size_t CHILDREN_SIZE = 20;
  
  enum NodeType {
    NODE_TAG  = 1,
    NODE_TEXT = 2
  };
  
  
  struct Node {
    uc_string text;
    uc_string attrs;
    NodeType type;
    
    Node* parent;
    vector<Node*> children;
    
    Node(uc_string text, NodeType type, Node* parent): Node(text, uc_string(), type, parent) {}
    Node(uc_string text, uc_string attrs, NodeType type, Node* parent): text(text), attrs(attrs), type(type), parent(parent) {
      children.reserve(CHILDREN_SIZE);
    }
    
    bool isRoot() {
      return parent == NULL;
    }
  };
  
  struct TagNode: public Node {
    TagNode(uc_string tag, Node* parent = NULL): Node(tag, NODE_TAG, parent) {}
    TagNode(uc_string tag, uc_string attrs, Node* parent = NULL): Node(tag, attrs, NODE_TAG, parent) {}
  };
  
  struct TextNode: public Node {
    TextNode(uc_string text = uc_string(), Node* parent = NULL): Node(text, NODE_TEXT, parent) {}
  };
  
  
private:
  uc_string inText;
  Node* globalRoot;
  
  string dictName;
  string headword;
  
  int nodesCount;
  int deletedNodesCount;
  
  
private:
  void deleteTree(Node* root);
  
  // Removes empty nodes (without children) and frees their memory
  // returns false if active node (root) is empty
  bool normalizeTree(Node* root);
  void addTagNode(Node*& root, list<TagNode*>& tagsStack, const uc_string& text, const uc_string& attrs);
  void addTextNode(Node* root, const uc_string& text);
  
  // Extracts tag name from open/close tag ([p] -> p)
  uc_string getTagName(const uc_string& tag);
  uc_string getTagAttrs(const uc_string& tag);
  void getTagData(const uc_string& tag, uc_string& tagName, uc_string& tagAttrs);
  void getTagData_V2(const uc_string& tag, uc_string& tagName, uc_string& tagAttrs);
  
  // Is tag ([m], [m1] - [m9])
  bool isParagraphTag(const uc_string& tag);
  
  // Tag to skip, but process its children
  bool isSkipTag(const uc_string& tag);
  
  // Tag to remove its content and children
  bool isRemoveTag(const uc_string& tag);
  
  // non DSL or unprocessed tag
  bool isUnknownTag(const uc_string& tag);
  
  bool isNewline(const uc_string& str);
  bool endsWithNewline(const uc_string& str);
  
  // Add <br> at the line break, skip first \n after end of paragraph tag (<p>)
  uc_string fixNewline(const uc_string& str, bool skipFirst = false);
  uc_string escapeHtml(const uc_string& str, bool strict = true);
  
  // Returns text until the newline or until the next paragraph tag
  // (used to write the text left at the end of unclosed paragraph tag)
  uc_string getTextToEndOfLine(const uc_string& str);
  
  void combineHtml(Node* root, uc_string& res);
  // Skips empty nodes (without children) [NOT_FINISHED]
  bool combineHtml_V2(Node* root, uc_string& res);
  void wrapHtml(uc_string& html);
  void processTree(Node* root);
  
  // -------- Utils --------
  void dumpStack(list<Node*> st);
  void dumpTree(Node* r, int lev, uc_string tagStart = uc_string(), uc_string tagEnd = uc_string(), bool closeTags = false, bool showRefs = false);
  void printWarning(const string& msg, bool sameLine = false, size_t pos = 0);


public:
  
  DslParser(const uc_string& str, const string& dictName, const string& headword);
  ~DslParser();
  
  uc_string generateHtml(bool dump_tree = false);
  
};

#endif
