#ifndef ARTICLE_DOM_H
#define ARTICLE_DOM_H

#include <string>
#include <list>
#include <stack>
#include <vector>

#include "vars.h"
#include "utils.h"


/// Parses the DSL language, representing it in its structural DOM form.
struct ArticleDom
{
  struct Node: public list<Node>
  {
    bool isTag; // true if it is a tag with subnodes, false if it's a leaf text // data.
    // Those are only used if isTag is true
    uc_string tagName;
    uc_string tagAttrs;
    uc_string text; // This is only used if isTag is false

    class Text {};
    class Tag {};

    Node( Tag, uc_string const & name, uc_string const & attrs ): isTag( true ),
      tagName( name ), tagAttrs( attrs )
    {}

    Node( Text, uc_string const & text_ ): isTag( false ), text( text_ )
    {}

    /// Concatenates all childen text nodes recursively to form all text
    /// the node contains stripped of any markup.
    uc_string renderAsText( bool stripTrsTag = false ) const;
  };

  /// Does the parse at construction. Refer to the 'root' member variable
  /// afterwards.
  ArticleDom( uc_string const &, const string& dictName = string(), const uc_string& headword_ = uc_string() );

  /// Root of DOM's tree
  Node root;

  void dumpDom() {
    dumpDom(root, 0);
  }
  
private:

  void openTag( uc_string const & name, uc_string const & attr, list< Node * > & stack );
  void closeTag( uc_string const & name, list< Node * > & stack, bool warn = true );

  bool atSignFirstInLine();
  void nextChar();
  
  void dumpDom(Node r, int lev);

  const uc_char* stringPos;
  const uc_char* lineStartPos;

  uc_char ch;
  bool escaped;
  unsigned transcriptionCount; // >0 = inside a [t] tag

  string dictionaryName;
  uc_string headword;
};

#endif