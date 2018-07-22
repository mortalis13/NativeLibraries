
#include "dsl_parser.h"


DslParser::DslParser(const uc_string& str, const string& dictName, const string& headword)
  : inText(str), dictName(dictName), headword(headword)
{
  globalRoot = new TagNode(U"");
}

DslParser::~DslParser() {
  deleteTree(globalRoot);
  
  // empty nodes are freed in the normalizeTree()
  if (nodesCount != deletedNodesCount) {
    printWarning("MEMORY: Created and deleted nodes count don't match, " + Utils::toStr(deletedNodesCount) + "/" + Utils::toStr(nodesCount));
  }
}

void DslParser::deleteTree(Node* root) {
  if (!root) return;
  for (Node* n: root->children) {
    deleteTree(n);
  }
  
  // Log::dg("~deleting '" + Utils::toStr(root) + "', '" + root->text + "'");
  delete root;
  deletedNodesCount++;
}


bool DslParser::normalizeTree(Node* root) {
  // nodesCount++;
  if (!root) return false;
  // Log::d("normalizeTree '" + Utils::toStr(root) + "', '" + root->text + "'");
  
  auto it = root->children.begin();
  while (it != root->children.end()) {
    // if current child is empty remove it and check if the root becomes empty after the deletion
    // the iterator is not incremented, the next child will be at the position of the removed node

    if (!normalizeTree(*it)) {
      // Log::d("deletingNodes '" + Utils::toStr(*it) + "', '" + (*it)->text + "'");
      Node* removeNode = *it;
      uc_string removedTag = removeNode->text;
      
      root->children.erase(it);
      delete removeNode;
      deletedNodesCount++;
      
      if (isParagraphTag(removedTag) && it != root->children.end()) {
        Node* nextNode = *it;
        if (nextNode->type == NODE_TEXT && Utils::isEmpty(nextNode->text)) {
          root->children.erase(it);
          delete nextNode;
          deletedNodesCount++;
        }
      }
      
      if (root->children.empty()) return false;
    }
    else it++;
  }
  
  if (root->type == NODE_TAG && root->children.empty()) return false;
  return true;
}


// Adds node to the tree and to the tags stack and enters it
void DslParser::addTagNode(Node*& root, list<TagNode*>& tagsStack, const uc_string& text, const uc_string& attrs) {
  TagNode* n = new TagNode(text, attrs, root);
  root->children.push_back(n);
  root = root->children.back();
  tagsStack.push_back(n);
  nodesCount++;
}

void DslParser::addTextNode(Node* root, const uc_string& text) {
  TextNode* n = new TextNode(text, root);
  root->children.push_back(n);
  nodesCount++;
}


uc_string DslParser::getTagName(const uc_string& tag) {
  uc_string tagName;
  uc_string tagAttrs;
  getTagData(tag, tagName, tagAttrs);
  return tagName;
}

uc_string DslParser::getTagAttrs(const uc_string& tag) {
  uc_string tagName;
  uc_string tagAttrs;
  getTagData(tag, tagName, tagAttrs);
  return tagAttrs;
}

void DslParser::getTagData(const uc_string& tag, uc_string& tagName, uc_string& tagAttrs) {
  uc_string tagNorm = tag;
  tagName = tag;
  
  size_t tagNameStart = tagNorm.find_first_not_of(U"[ /");
  size_t tagNameEnd = tagNorm.find_first_of(U" ]", tagNameStart);
  
  size_t tagAttrsStart = tagNorm.find_first_not_of(U" ]", tagNameEnd);
  size_t tagAttrsEnd = tagNorm.find_first_of(U"]", tagAttrsStart);
  
  // Log::d("%s:\t\t[%d:%d],\t\t[%d:%d]", tagNorm.c_str(), tagNameStart, tagNameEnd, tagAttrsStart, tagAttrsEnd);
  
  if (tagNameStart != npos)
    tagName = tagNorm.substr(tagNameStart, tagNameEnd - tagNameStart);
  
  if (tagAttrsStart != npos)
    tagAttrs = tagNorm.substr(tagAttrsStart, tagAttrsEnd - tagAttrsStart);
  
  if (!tagName.empty() && (tagName[0] == U' ' || tagName[tagName.size()-1] == U' ')) Utils::trim(tagName);
  if (!tagAttrs.empty() && (tagAttrs[0] == U' ' || tagAttrs[tagAttrs.size()-1] == U' ')) Utils::trim(tagAttrs);
}

void DslParser::getTagData_V2(const uc_string& tag, uc_string& tagName, uc_string& tagAttrs) {
  uc_string tagNorm = tag;
  if (tag.find(U'[') != npos) tagNorm.erase(tagNorm.find_first_of(U'['), 1);
  if (tag.find(U']') != npos) tagNorm.erase(tagNorm.find_last_of(U']'), 1);
  Utils::trim(tagNorm);
  
  bool attrStarted = false;
  
  size_t len = tagNorm.size();
  for (size_t pos = 0; pos < len; pos++) {
    if (tagNorm[pos] == U'/') continue;
    
    if (attrStarted) {
      tagAttrs += tagNorm[pos];
    }
    else if (tagNorm[pos] == U' ') {
      attrStarted = true;
      size_t attrPos = tagNorm.find_first_not_of(U' ', pos);
      if (attrPos != npos) pos = attrPos - 1;
    }
    else {
      tagName += tagNorm[pos];
    }
  }
}

bool DslParser::isParagraphTag(const uc_string& tag) {
  uc_string tagName = getTagName(tag);
  return tagName.size() == 2 && tagName[0] == U'm' || tagName == U"m";
}

bool DslParser::isSkipTag(const uc_string& tag) {
  return find(SKIP_TAGS.cbegin(), SKIP_TAGS.cend(), tag) != SKIP_TAGS.end();
}

bool DslParser::isRemoveTag(const uc_string& tag) {
  return find(REMOVE_TAGS.cbegin(), REMOVE_TAGS.cend(), tag) != REMOVE_TAGS.end();
}

bool DslParser::isUnknownTag(const uc_string& tag) {
  return !tag.empty() && find(DSL_TAGS.cbegin(), DSL_TAGS.cend(), tag) == DSL_TAGS.end();
}

bool DslParser::isNewline(const uc_string& str) {
  return str == U"\n" || str == U"\r\n";
}

bool DslParser::endsWithNewline(const uc_string& str) {
  size_t len = str.size();
  return !str.empty() && (str[len-1] == '\n' || str[len-1] == '\r');
}


uc_string DslParser::fixNewline(const uc_string& str, bool skipFirst) {
  if (str.find(U"\n") == npos) return str;
  
  uc_string res;
  
  size_t len = str.size();
  for (size_t pos = 0; pos < len; pos++) {
    if (str[pos] == '\r') continue;
    if (str[pos] == '\n') {
      if (skipFirst) skipFirst = false;
      else res += U"<br>";
    }
    res.push_back(str[pos]);
  }
  
  return res;
}

uc_string DslParser::escapeHtml(const uc_string& str, bool strict) {
  if (str.empty()) return str;
  uc_string result = str;

  size_t i = result.size()-1;
  while (true) {
    switch (result[i]) {
      case U'<':
        result.erase(i, 1);
        result.insert(i, U"&lt;");
        break;

      case U'>':
        result.erase(i, 1);
        result.insert(i, U"&gt;");
        break;
        
      case U'&':
        if (strict) {
          result.erase(i, 1);
          result.insert(i, U"&amp;");
        }
        break;

      case U'"':
        if (strict) {
          result.erase(i, 1);
          result.insert(i, U"&#34;");
        }
        break;

      case U'\'':
        if (strict) {
          result.erase(i, 1);
          result.insert(i, U"&#39;");
        }
        break;
    }
    
    if (i == 0) break;
    i--;
  }

  return result;
}

uc_string DslParser::getTextToEndOfLine(const uc_string& str) {
  uc_string res = str;
  size_t pos = str.find_first_of(U"\r\n");
  if (pos != npos) {
    res = str.substr(0, pos);
  }
  
  return res;
}


void DslParser::dumpStack(list<Node*> st) {
  while (!st.empty()) {
    Log::dg(st.back()->text); st.pop_back();
  }
}

void DslParser::dumpTree(Node* root, int lev, uc_string tagStart, uc_string tagEnd, bool closeTags, bool showRefs) {
  uc_string pad;
  for (int i=0; i<lev; i++) pad += U"  ";
  
  for (Node* n: root->children) {
    if (n->type == NODE_TEXT) {
      uc_string text = n->text;
      if (showRefs) text += U" " + Utils::convert(Utils::toStr(n));
      // Log::dg(pad + text);
      Log::dg(pad + U"'" + n->text + U"'");
    }
    
    if (n->type == NODE_TAG) {
      uc_string attrs = n->attrs;
      if (!attrs.empty()) attrs = U" " + attrs;

      uc_string text = pad + tagStart + n->text + attrs + tagEnd;
      if (showRefs) text += U" " + Utils::convert(Utils::toStr(n));
      
      Log::dg(text);
      dumpTree(n, lev+1, tagStart, tagEnd, closeTags, showRefs);
      
      if (closeTags) {
        Log::dg(pad + tagStart + U"/" + n->text + tagEnd);
      }
    }
  }
}

void DslParser::printWarning(const string& msg, bool sameLine, size_t pos) {
  string text = "[WARNING]: [" + dictName + "; " + headword + "]    \t" + msg;
  if (pos != 0) {
    Log::dg(text + ", pos: %d", pos);
  }
  else if (sameLine) Log::dgn(text);
  else Log::dg(text);
}


bool DslParser::combineHtml_V2(Node* root, uc_string& res) {
  bool notEmpty = true;
  
  bool afterParagraph = false;
  bool afterRemoveTag = false;
  
  // process [ref] apart, need to include link text to 'href' and inside the tag
  if (root->type == NODE_TAG && root->text == U"ref") {
    uc_string linkText;
    
    uc_string openTag = U"a href='word:///";
    uc_string closeTag = U"a";
    
    for (Node* n: root->children) {
      if (n->type == NODE_TEXT) {
        linkText += n->text;
      }
    }
    
    openTag = HTML_TAG_START + openTag + escapeHtml(linkText) + U"'" + HTML_TAG_END;
    closeTag = HTML_TAG_START + U"/" + closeTag + HTML_TAG_END;
    
    res += openTag;
    res += linkText;
    res += closeTag;
    
    return notEmpty;
  }
  
  for (Node* n: root->children) {
    if (n->type == NODE_TEXT) {
      uc_string text = n->text;
      
      // if tag was removed, skip its HTML newline (<br> tag) but leave \n for the code formatting
      // add <br> after each line except <p> tags
      if (!afterRemoveTag) {
        text = fixNewline(text, afterParagraph);
      }
      
      res += text;
    }
    
    afterRemoveTag = false;
    afterParagraph = false;
    
    if (n->type == NODE_TAG) {
      uc_string tag = n->text;
      uc_string openTag = tag;
      uc_string closeTag = tag;
      
      if (tag.empty()) continue;
      
      if (isRemoveTag(tag)) {
        // skip the tag and its children
        afterRemoveTag = true;
      }
      else {
        // set correct HTML tags, attributes depending on DSL tag type
        if (tag == U"p") {
          openTag = U"span class='label'";
          closeTag = U"span";
        }
        else if (tag[0] == U'm') {
          uc_string tagLev = U"0";
          if (tag.size() == 2) {
            tagLev = uc_string(1, tag[1]);
          }
          
          openTag = U"p class='line p" + tagLev + U"'";
          closeTag = U"p";
          afterParagraph = true;
        }
        else if (tag == U"ex") {
          openTag = U"cite class='example'";
          closeTag = U"cite";
        }
        else if (tag == U"c") {
          uc_string color = U"green";
          uc_string attrs = escapeHtml(n->attrs);
          if (!attrs.empty()) color = attrs;
          openTag = U"font color='" + color + U"'";
          if (attrs.empty()) openTag += U" class='color-default'";
          closeTag = U"font";
        }
        else if (tag == U"ref") {
          // will be processed above
          combineHtml_V2(n, res);
          continue;
        }
        else if (isUnknownTag(tag)) {
          printWarning("Unknown tag: " + Utils::convert(tag));
        }
        
        openTag = HTML_TAG_START + openTag + HTML_TAG_END;
        closeTag = HTML_TAG_START + U"/" + closeTag + HTML_TAG_END;
        
        if (n->children.empty()) return false;
        
        if (isSkipTag(tag)) {
          openTag = U"";
          closeTag = U"";
        }
        
        res += openTag;
        if (!combineHtml_V2(n, res) && n->children.size() == 1) {
          res = res.substr(0, res.size() - openTag.size());
          notEmpty = false;
        }
        else res += closeTag;
      }
    }
  } // for
  
  return notEmpty;
}


void DslParser::combineHtml(Node* root, uc_string& res) {
  bool afterParagraph = false;
  bool afterRemoveTag = false;
  bool afterUnknownTag = false;
  bool inUnknownTag = false;
  
  uc_string rootTag = root->text;
  
  inUnknownTag = !root->isRoot() && isUnknownTag(rootTag);
  if (inUnknownTag) printWarning("Unknown tag: " + Utils::convert(rootTag));
  
  // process [ref] apart, need to include link text to 'href' and inside the tag
  if (root->type == NODE_TAG && (rootTag== U"ref" || rootTag== U"url")) {
    uc_string linkText;
    
    uc_string openTag = U"a href='word:///";
    if (rootTag== U"url") openTag = U"a href='";
    uc_string closeTag = U"a";
    
    for (Node* n: root->children) {
      if (n->type == NODE_TEXT) {
        linkText += n->text;
      }
    }
    
    uc_string linkAddr;
    linkAddr = linkText;
    if (rootTag == U"ref") linkAddr = escapeHtml(linkText);
    
    openTag = HTML_TAG_START + openTag + linkAddr + U"'" + HTML_TAG_END;
    closeTag = HTML_TAG_START + U"/" + closeTag + HTML_TAG_END;
    
    res += openTag + linkText + closeTag;
    
    return;
  }
  
  for (Node* n: root->children) {
    if (n->type == NODE_TEXT) {
      uc_string text = n->text;
      text = escapeHtml(text, false);
      
      // Log::dg("text of [" + root->text + "], {" + Utils::toStr(text.size()) + "}");
      // Log::dg("afterRemoveTag: %d, afterUnknownTag: %d, inUnknownTag: %d", afterRemoveTag, afterUnknownTag, inUnknownTag);
      
      // if tag was removed, skip its HTML newline (<br> tag) but leave \n for the code formatting
      // add <br> after each line except <p> tags
      if (!afterRemoveTag && !afterUnknownTag && !inUnknownTag) {
        text = fixNewline(text, afterParagraph);
      }
      
      Utils::fixUnicode(text);
      res += text;
    }
    
    afterRemoveTag = false;
    afterParagraph = false;
    afterUnknownTag = false;
    
    if (n->type == NODE_TAG) {
      uc_string tag = n->text;
      uc_string openTag = tag;
      uc_string closeTag = tag;
      
      if (tag.empty()) continue;
      
      if (isSkipTag(tag)) {
        // skip tag itself and go to its children
        combineHtml(n, res);
      }
      else if (isRemoveTag(tag)) {
        // skip the tag and its children
        afterRemoveTag = true;
      }
      else {
        // set correct HTML tags, attributes depending on DSL tag type
        if (tag == U"p") {
          openTag = U"span class='label'";
          closeTag = U"span";
        }
        else if (tag[0] == U'm') {
          uc_string tagLev = U"0";
          if (tag.size() == 2) {
            tagLev = uc_string(1, tag[1]);
          }
          
          openTag = U"p class='line p" + tagLev + U"'";
          closeTag = U"p";
          afterParagraph = true;
        }
        else if (tag == U"ex") {
          openTag = U"cite class='example'";
          closeTag = U"cite";
        }
        else if (tag == U"c") {
          uc_string color = U"green";
          uc_string attrs = escapeHtml(n->attrs);
          if (!attrs.empty()) color = attrs;
          openTag = U"font color='" + color + U"'";
          if (attrs.empty()) openTag += U" class='color-default'";
          closeTag = U"font";
        }
        else if (tag == U"ref" || tag == U"url") {
          // will be processed above
          combineHtml(n, res);
          continue;
        }
        else if (isUnknownTag(tag)) {
          afterUnknownTag = true;
        }
        
        openTag = HTML_TAG_START + openTag + HTML_TAG_END;
        closeTag = HTML_TAG_START + U"/" + closeTag + HTML_TAG_END;
        
        res += openTag;
        combineHtml(n, res);
        res += closeTag;
      }
    }
  } // for
}


void DslParser::processTree(Node* root) {
  list<TagNode*> tagsStack;
  stack<TagNode*> tempStack;
  
  uc_string tag;
  uc_string text;
  
  bool tagStarted = false;
  bool escapeFound = false;
  bool linkStartFound = false;
  bool linkEndFound = false;
  
  // go char by char
  size_t len = inText.size();
  for (size_t strPos = 0; strPos < len; strPos++) {
    uc_char c = inText[strPos];
    
    // <<>> to [ref][/ref]
    if (c == U'<' && !escapeFound) {
      if (strPos != len-1 && inText[strPos+1] == U'<') {
        linkStartFound = true;
        strPos++;
      }
    }
    else if (c == U'>' && !escapeFound) {
      if (strPos != len-1 && inText[strPos+1] == U'>') {
        linkEndFound = true;
        strPos++;
      }
    }
    
    if (tagStarted && !escapeFound || linkStartFound || linkEndFound) {
      tag.push_back(c);
      
      if (c == U']' || linkStartFound || linkEndFound) {
        // found open or close tag ([p] or [/p])
        tagStarted = false;
        
        if (linkStartFound) tag = U"[ref]";
        if (linkEndFound)   tag = U"[/ref]";
        linkStartFound = linkEndFound = false;
        
        bool isOpenTag = true;
        if (tag[1] == U'/') isOpenTag = false;
        
        if (isOpenTag) {
          // extracts unclosed tags from the stack, exits from them
          // then adds current paragraph tag [m] and puts the unclosed tags inside the [m] tag
          // and also puts those tags above the [m] in the stack
          
          if (isParagraphTag(tag)) {
            uc_string parText = getTextToEndOfLine(text);
            if (parText == text) text.clear();
            else text = text.substr(parText.size());
            
            if (!parText.empty()) {
              addTextNode(root, parText);
            }
            
            while (!tagsStack.empty()) {
              tempStack.push(tagsStack.back()); tagsStack.pop_back();
              root = root->parent;
              
              if (isParagraphTag(tempStack.top()->text)) {
                tempStack.pop();
                break;
              }
            }
          }
          
          // add current text as child, clear it, add new tag node as child and enter this node
          if (!text.empty()) {
            addTextNode(root, text);
            text.clear();
          }
          
          addTagNode(root, tagsStack, getTagName(tag), getTagAttrs(tag));
          
          if (isParagraphTag(tag)) {
            // the second part of unclosed tags restoring (put the tags inside the [m])
            while (!tempStack.empty()) {
              TagNode* tempNode = tempStack.top(); tempStack.pop();
              addTagNode(root, tagsStack, tempNode->text, tempNode->attrs);
            }
          }
        }
        else {
          // find corresponding open tag and exit the current root (tag node)
          bool isTagMatch = false;
          bool isParagraph = false;
          bool crossTags = false;
          
          // search the tags stack until open-close tags match
          // save unmatched tags to temp stack and restore them later
          while (!isTagMatch && !tagsStack.empty()) {
            TagNode* openNode = tagsStack.back(); tagsStack.pop_back();

            uc_string openTagName = openNode->text;
            uc_string closeTagName = getTagName(tag);
            
            // tag names match or paragraph tag ([m1][/m], [m][/m])
            isParagraph = isParagraphTag(openTagName) && isParagraphTag(tag);
            isTagMatch = openTagName == closeTagName || isParagraph;
            
            // unbalanced tags, ok for DSL, save them for later use
            if (!isTagMatch) {
              crossTags = true;
              tempStack.push(openNode);
            }
            
            bool removeChild = false;
            
            // add not empty text as child or remove the empty node (no text, no children)
            if (!text.empty()) {
              addTextNode(root, text);
              text.clear();
            }
            
            if (!root) {
              printWarning("root NULL [1]");
              continue;
            }
            
            // exit the node 
            // (if it's direct match just closes it, if some tags were unclosed, they're closed here to keep the balance)
            root = root->parent;
          }
          
          // found close tag but no open tag was added earlier to the stack
          if (!isTagMatch) printWarning("No open tag for: " + Utils::convert(tag), false, strPos);
          
          while (!tempStack.empty()) {
            // move temp nodes to the main tags stack
            TagNode* tempNode = tempStack.top(); tempStack.pop();
            
            if (!isTagMatch) {
              // set root as the top restored node
              tagsStack.push_back(tempNode);
              root = tempNode;
            }
            else {
              // the tag was opened but not closed before current closing tag
              // (crossTags is true, unbalanced tags case)
              // so this tag (and all other in the temp stack) is automatically closed before the current 'tag'
              // and to keep the balance it need to be opened after the current close tag
              // so this tag is copied to the parent of the closed 'tag'
              // -----
              // for input '[p][i][c]n[/i][/p]' it creates '[p][i][c]n[/c][/i][c][/c][/p]'
              // so the [c] tag is now balanced
              // if the [/c] is after [/p] in the input text then in the later iterations
              // it [c] will be copied after [p], to the parent of [p],
              // if [/c] is not in the text all the other tags will be its children 
              // until the end of the input text or until another closing tag
              
              addTagNode(root, tagsStack, tempNode->text, tempNode->attrs);
            }
          }
        }
        
        tag.clear();
      }
    }
    else if (c == U'[' && !escapeFound) {
      tagStarted = true;
      tag.push_back(c);
    }
    else if (c == '\\' && !escapeFound) {
      escapeFound = true;
    }
    else {
      // text inside/outside tags
      escapeFound = false;
      text.push_back(c);
    }
  }
  
  // add last text after all tags
  if (!text.empty()) {
    Utils::trimRight(text);
    addTextNode(root, text);
    text.clear();
  }
  
  if (!tagsStack.empty()) {
    printWarning("Unclosed tags: ", true);
    while (!tagsStack.empty()) {
      Log::dgn(tagsStack.back()->text + U" "); tagsStack.pop_back();
    }
    Log::dgn("\n");
  }
}


uc_string DslParser::generateHtml(bool dump_tree) {
  uc_string html;
  
  try {
    nodesCount = 1;         // ROOT
    deletedNodesCount = 0;
    
    processTree(globalRoot);
    // dumpTree(globalRoot, 0, "[", "]", false, false);
    normalizeTree(globalRoot);
    combineHtml(globalRoot, html);
    
    if (!html.empty()) wrapHtml(html);
    
    // dump_tree = true;
    if (dump_tree) {
      Log::dg("---TreeDump---");
      dumpTree(globalRoot, 0, U"[", U"]", false, false);
      Log::dg("---TreeEnd---\n");
    }
  }
  catch (const std::exception & e) {
    Log::dg("[ERROR]: DSL text parsing: [%s, %s], %s\n", dictName.c_str(), headword.c_str(), e.what());
  }
  
  return html;
}

void DslParser::wrapHtml(uc_string& html) {
  html = U"<div class='article'>\n<div class='dict-name'>" + Utils::convert(dictName) + U"</div>\n" + html + U"\n</div>\n";
}
