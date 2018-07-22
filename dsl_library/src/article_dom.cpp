
#include "article_dom.h"

#include "dsl_utils.h"
#include "folding.h"


// Returns true if src == 'm' and dest is 'mX', where X is a digit
static inline bool checkM(uc_string const & dest, uc_string const & src) {
  return (src == U"m" && dest.size() == 2 && dest[0] == U'm');
}

ArticleDom::ArticleDom(uc_string const & str, const string& dictName, const uc_string& headword_) :
    root(Node::Tag(), uc_string(), uc_string()), stringPos(str.c_str()), lineStartPos(str.c_str()),
    transcriptionCount(0), dictionaryName(dictName), headword(headword_)
{
  list<Node*> stack; // Currently opened tags
  Node* textNode = 0; // A leaf node which currently accumulates text.

  try {
    for (;;) {
      nextChar();

      if (ch == U'@' && !escaped) {
        if (!atSignFirstInLine()) {
          // Not insided card
          if (dictName.empty()) Log::d("Unescaped '@' symbol found");
          else Log::d("Unescaped '@' symbol found in \"%s\"", dictName.c_str());
        }
        else {
          // Insided card
          uc_string linkTo;
          nextChar();
          for (;; nextChar()) {
            if (ch == U'\n') break;
            if (ch != U'\r') linkTo.push_back(ch);
          }
          linkTo = Folding::trimWhitespace(linkTo);

          if (!linkTo.empty()) {
            list < uc_string > allLinkEntries;
            DslUtils::expandOptionalParts(linkTo, &allLinkEntries);

            for (list<uc_string>::iterator entry = allLinkEntries.begin();
                entry != allLinkEntries.end();) {
              if (!textNode) {
                Node text = Node(Node::Text(), uc_string());

                if (stack.empty()) {
                  root.push_back(text);
                  stack.push_back(&root.back());
                }
                else {
                  stack.back()->push_back(text);
                  stack.push_back(&stack.back()->back());
                }

                textNode = stack.back();
              }
              textNode->text.push_back(U'-');
              textNode->text.push_back(U' ');

              // Close the currently opened text node
              stack.pop_back();
              textNode = 0;

              uc_string linkText = Folding::trimWhitespace(*entry);
              DslUtils::processUnsortedParts(linkText, true);
              ArticleDom nodeDom(linkText, dictName, headword_);

              Node link(Node::Tag(), U"@", uc_string());
              for (Node::iterator n = nodeDom.root.begin();
                  n != nodeDom.root.end(); ++n)
                link.push_back(*n);

              ++entry;

              if (stack.empty()) {
                root.push_back(link);
                if (entry != allLinkEntries.end()) // Add line break before next entry
                  root.push_back(Node(Node::Tag(), U"br", uc_string()));
              }
              else {
                stack.back()->push_back(link);
                if (entry != allLinkEntries.end())
                  stack.back()->push_back(Node(Node::Tag(), U"br", uc_string()));
              }
            }

            // Skip to next '@'

            while (!(ch == U'@' && !escaped && atSignFirstInLine()))
              nextChar();

            stringPos--;
            ch = U'\n';
            escaped = false;
          }
        }
      } // if ( ch == U'@' )


      if (ch == U'[' && !escaped) {
        // Beginning of a tag.
        do {
          nextChar();
        } while (Folding::isWhitespace(ch));

        bool isClosing = false;
        if (ch == U'/' && !escaped) {
          // A closing tag.
          isClosing = true;
          nextChar();
        }

        // Read tag's name
        uc_string name;
        while ((ch != U']' || escaped) && !Folding::isWhitespace(ch)) {
          name.push_back(ch);
          nextChar();
        }
        
        while (Folding::isWhitespace (ch))
          nextChar();

        // Read attrs
        uc_string attrs;
        while (ch != U']' || escaped) {
          attrs.push_back(ch);
          nextChar();
        }

        // Add the tag, or close it
        if (textNode) {
          // Close the currently opened text node
          stack.pop_back();
          textNode = 0;
        }

        // If the tag is [t], we update the transcriptionCount
        if (name == U"t") {
          if (isClosing) {
            if (transcriptionCount) --transcriptionCount;
          }
          else ++transcriptionCount;
        }

        if (!isClosing) {
          if ( name == U"m" || (name.size() == 2 && name[0] == U'm' && iswdigit(name[1])) ) {
            // Opening an 'mX' or 'm' tag closes any previous 'm' tag
            closeTag(U"m", stack, false);
          }
          
          openTag(name, attrs, stack);
          if (name == U"br") {
            // [br] tag don't have closing tag
            closeTag(name, stack);
          }
        }
        else {
          closeTag(name, stack);
        }
        
        continue;
      } // if ( ch == '[' )


      if (ch == U'<' && !escaped) {
        // Special case: the <<name>> link

        nextChar();

        if (ch != U'<' || escaped) {
          // Ok, it's not it.
          --stringPos;

          if (escaped) {
            --stringPos;
            escaped = false;
          }
          ch = U'<';
        }
        else {
          // Get the link's body
          do {
            nextChar();
          } while (Folding::isWhitespace(ch));

          uc_string linkTo, linkText;

          for (;; nextChar()) {
            // Is it the end?
            if (ch == U'>' && !escaped) {
              nextChar();

              if (ch == U'>' && !escaped)
                break;
              else {
                linkTo.push_back(U'>');
                linkTo.push_back(ch);

                linkText.push_back(U'>');
                if (escaped) linkText.push_back(U'\\');
                linkText.push_back(ch);
              }
            }
            else {
              linkTo.push_back(ch);

              if (escaped) linkText.push_back(U'\\');
              linkText.push_back(ch);
            }
          }

          // Add the corresponding node

          if (textNode) {
            // Close the currently opened text node
            stack.pop_back();
            textNode = 0;
          }

          linkText = Folding::trimWhitespace(linkText);
          DslUtils::processUnsortedParts(linkText, true);
          ArticleDom nodeDom(linkText, dictName, headword_);

          Node link(Node::Tag(), U"ref", uc_string());
          for (Node::iterator n = nodeDom.root.begin(); n != nodeDom.root.end();
              ++n)
            link.push_back(*n);

          if (stack.empty())
            root.push_back(link);
          else stack.back()->push_back(link);

          continue;
        }
      } // if ( ch == '<' )

      if (ch == U'{' && !escaped) {
        // Special case: {{comment}}

        nextChar();

        if (ch != U'{' || escaped) {
          // Ok, it's not it.
          --stringPos;

          if (escaped) {
            --stringPos;
            escaped = false;
          }
          ch = U'{';
        }
        else {
          // Skip the comment's body
          for (;;) {
            nextChar();

            // Is it the end?
            if (ch == U'}' && !escaped) {
              nextChar();

              if (ch == U'}' && !escaped) break;
            }
          }

          continue;
        }
      } // if ( ch == '{' )


      // If we're here, we've got a normal symbol, to be saved as text.
      // If there's currently no text node, open one
      
      if (!textNode) {
        Node text = Node(Node::Text(), uc_string());

        if (stack.empty()) {
          root.push_back(text);
          stack.push_back(&root.back());
        }
        else {
          stack.back()->push_back(text);
          stack.push_back(&stack.back()->back());
        }
        
        textNode = stack.back();
      }

      // If we're inside the transcription, do old-encoding conversion
      if (transcriptionCount) {
        switch (ch) {
          case 0x2021: ch = 0xE6; break;
          case 0x407: ch = 0x72; break;
          case 0xB0: ch = 0x6B; break;
          case 0x20AC: ch = 0x254; break;
          case 0x404: ch = 0x7A; break;
          case 0x40F: ch = 0x283; break;
          case 0xAB: ch = 0x74; break;
          case 0xAC: ch = 0x64; break;
          case 0x2020: ch = 0x259; break;
          case 0x490: ch = 0x6D; break;
          case 0xA7: ch = 0x66; break;
          case 0xAE: ch = 0x6C; break;
          case 0xB1: ch = 0x67; break;
          case 0x45E: ch = 0x65; break;
          case 0xAD: ch = 0x6E; break;
          case 0xA9: ch = 0x73; break;
          case 0xA6: ch = 0x77; break;
          case 0x2026: ch = 0x28C; break;
          case 0x452: ch = 0x76; break;
          case 0x408: ch = 0x70; break;
          case 0x40C: ch = 0x75; break;
          case 0x406: ch = 0x68; break;
          case 0xB5: ch = 0x61; break;
          case 0x491: ch = 0x25B; break;
          case 0x40A: ch = 0x14B; break;
          case 0x2030: ch = 0xF0; break;
          case 0x456: ch = 0x6A; break;
          case 0xA4: ch = 0x62; break;
          case 0x409: ch = 0x292; break;
          case 0x40E: ch = 0x69; break;
          //case 0x44D: ch = 0x131; break;
          case 0x40B: ch = 0x4E8; break;
          case 0xB6: ch = 0x28A; break;
          case 0x2018: ch = 0x251; break;
          case 0x457: ch = 0x265; break;
          case 0x458: ch = 0x153; break;
          case 0x405: textNode->text.push_back( 0x153 ); ch = 0x303; break;
          case 0x441: ch = 0x272; break;
          case 0x442: textNode->text.push_back( 0x254 ); ch = 0x303; break;
          case 0x443: ch = 0xF8; break;
          case 0x445: textNode->text.push_back(0x25B ); ch = 0x303; break;
          case 0x446: ch = 0xE7; break;
          case 0x44C: textNode->text.push_back( 0x251 ); ch = 0x303; break;
          case 0x44D: ch = 0x26A; break;
          case 0x44F: ch = 0x252; break;
          case 0x30: ch = 0x3B2; break;
          case 0x31: textNode->text.push_back( 0x65 ); ch = 0x303; break;
          case 0x32: ch = 0x25C; break;
          case 0x33: ch = 0x129; break;
          case 0x34: ch = 0xF5; break;
          case 0x36: ch = 0x28E; break;
          case 0x37: ch = 0x263; break;
          case 0x38: ch = 0x1DD; break;
          case 0x3A: ch = 0x2D0; break;
          case 0x27: ch = 0x2C8; break;
          case 0x455: ch = 0x1D0; break;
          case 0xB7: ch = 0xE3; break;
  
          case 0x00a0: ch = 0x02A7; break;
          //case 0x00b1: ch = 0x0261; break;
          case 0x0402: textNode->text.push_back( 0x0069 ); ch = U':'; break;
          case 0x0403: textNode->text.push_back( 0x0251 ); ch = U':'; break;
          //case 0x040b: ch = 0x03b8; break;
          //case 0x040e: ch = 0x026a; break;
          case 0x0428: ch = 0x0061; break;
          case 0x0453: textNode->text.push_back( 0x0075 ); ch = U':'; break;
          case 0x201a: ch = 0x0254; break;
          case 0x201e: ch = 0x0259; break;
          case 0x2039: textNode->text.push_back( 0x0064 ); ch = 0x0292; break;
        }
      }

      if (escaped && ch == U' ') ch = 0xA0; // Escaped spaces turn into non-breakable ones in Lingvo

      textNode->text.push_back(ch);
    } // for( ; ; )
  }
  catch (const std::exception &exc) {
    // Log::d("--ArticleDom Exception, " + string(exc.what()));
  }

  if (textNode) stack.pop_back();

  if (stack.size()) {
    Log::d("Warning: %u tags were unclosed.\n", (unsigned) stack.size());
  }
}

void ArticleDom::openTag(uc_string const & name, uc_string const & attrs, list<Node *> &stack) {
  list<Node> nodesToReopen;

  if (name == U"m" || checkM(name, U"m")) {
    // All tags above [m] tag will be closed and reopened after
    // to avoid break this tag by closing some other tag.

    while (stack.size()) {
      nodesToReopen.push_back(Node(Node::Tag(), stack.back()->tagName, stack.back()->tagAttrs));

      if (stack.back()->empty()) {
        // Empty nodes are deleted since they're no use
        stack.pop_back();
        Node * parent = stack.size() ? stack.back() : &root;
        parent->pop_back();
      }
      else stack.pop_back();
    }
  }

  // Add tag
  Node node(Node::Tag(), name, attrs);

  if (stack.empty()) {
    root.push_back(node);
    stack.push_back(&root.back());
  }
  else {
    stack.back()->push_back(node);
    stack.push_back(&stack.back()->back());
  }

  // Reopen tags if needed
  while (nodesToReopen.size()) {
    if (stack.empty()) {
      root.push_back(nodesToReopen.back());
      stack.push_back(&root.back());
    }
    else {
      stack.back()->push_back(nodesToReopen.back());
      stack.push_back(&stack.back()->back());
    }

    nodesToReopen.pop_back();
  }
}


void ArticleDom::closeTag(uc_string const & name, list<Node *> & stack, bool warn) {
  // Find the tag which is to be closed
  list<Node *>::reverse_iterator n;
  for (n = stack.rbegin(); n != stack.rend(); ++n) {
    if ((*n)->tagName == name || checkM((*n)->tagName, name)) {
      // Found it
      break;
    }
  }

  if (n != stack.rend()) {
    // If there is a corresponding tag, close all tags above it,
    // then close the tag itself, then reopen all the tags which got closed.
    list<Node> nodesToReopen;

    while (stack.size()) {
      bool found = stack.back()->tagName == name || checkM(stack.back()->tagName, name);

      if (!found) {
        nodesToReopen.push_back(Node(Node::Tag(), stack.back()->tagName, stack.back()->tagAttrs));
      }

      if (stack.back()->empty() && stack.back()->tagName != U"br") {
        // Empty nodes except [br] tag are deleted since they're no use
        stack.pop_back();
        Node* parent = stack.size() ? stack.back() : &root;
        parent->pop_back();
      }
      else {
        stack.pop_back();
      }

      if (found) break;
    }

    while (nodesToReopen.size()) {
      if (stack.empty()) {
        root.push_back(nodesToReopen.back());
        stack.push_back(&root.back());
      }
      else {
        stack.back()->push_back(nodesToReopen.back());
        stack.push_back(&stack.back()->back());
      }

      nodesToReopen.pop_back();
    }
  }
  else if (warn) {
    Log::d("No corresponding opening tag for closing tag \"%s\" found.", Utils::convertBytes(name).c_str());
  }
}

void ArticleDom::nextChar() {
  if (!*stringPos) throw runtime_error("nextChar() [1]");

  ch = *stringPos++;
  // cout << ch << " ";

  if (ch == U'\\') {
    if (!*stringPos) throw runtime_error("nextChar() [2]");
    ch = *stringPos++;
    escaped = true;
  }
  else if (ch == U'[' && *stringPos == U'[') {
    ++stringPos;
    escaped = true;
  }
  else if (ch == U']' && *stringPos == U']') {
    ++stringPos;
    escaped = true;
  }
  else escaped = false;

  if (ch == '\n' || ch == '\r') lineStartPos = stringPos;
}

bool ArticleDom::atSignFirstInLine() {
  // Check if '@' sign is first after '\n', leading spaces and dsl tags
  if (stringPos <= lineStartPos) return true;
  return DslUtils::findSubentryChar(uc_string(lineStartPos)) != npos;
}


uc_string ArticleDom::Node::renderAsText(bool stripTrsTag) const {
  if (!isTag) return text;
  uc_string result;

  for (list<Node>::const_iterator i = begin(); i != end(); ++i)
    if (!stripTrsTag || i->tagName != U"!trs")
      result += i->renderAsText(stripTrsTag);

  return result;
}

void ArticleDom::dumpDom(Node r, int lev) {
  uc_string pad;
  for (int i=0;i<lev;i++) pad += U"  ";
  
  for (Node n: r) {
    // if (n.isTag) Log::d(n.tagName);
    
    if (!n.isTag) {
      Log::d(pad + n.text);
    }
    
    if (n.isTag) {
      Log::d(pad + U"[" + n.tagName + U"]");
      dumpDom(n, lev+1);
      // Log::d(pad + U"</" + n.tagName + U">");
    }
  }
}
