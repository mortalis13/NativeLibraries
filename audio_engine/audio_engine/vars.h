#ifndef WAV_VARS_H
#define WAV_VARS_H

#include <string>
#include <map>
#include <stdexcept>

using std::runtime_error;
using std::string;


// typedef char16_t uc_char;
// typedef std::u16string uc_string;

typedef char32_t uc_char;
typedef std::u32string uc_string;
typedef string::size_type pos_t;


static const pos_t npos = string::npos;

enum Encoding {
  ENC_UTF16LE = 1,
  ENC_UTF8    = 2,
  ENC_UCS4LE  = 4,
};

namespace Vars {

static const char* DSL_ENCODING = "UTF-16LE";
static const char* DSL_ENCODING_UTF8 = "UTF-8";
static const char* DATA_ENCODING = "UCS-4LE";
static const char* STR_ENCODING = "UTF-8";

static const string ERROR_BTREE_DECOMPRESS_NODE = "Error decompressing B-tree node";
static const string ERROR_BTREE_COMPRESS_NODE = "Error compressing B-tree node";
static const string ERROR_BTREE_CORRUPT_LEAF = "Corrupted chain data in a B-tree leaf";
static const string ERROR_INDEX_NOT_OPENED = "Index not opened";
static const string ERROR_CONVERT_STRING = "Error converting string";
static const string ERROR_DECOMPRESS_CHUNK = "Error decompressing chunk";
static const string ERROR_COMPRESS_CHUNK = "Error compressing chunk";
static const string ERROR_CHUNK_ADDRESS_OUT_OF_RANGE = "Chunked address out of range";
static const string ERROR_CANNOT_OPEN_FILE = "Cannot open file";
static const string ERROR_READ_FILE = "Error reading file";
static const string ERROR_CANNOT_OPEN_DSL_FILE = "Cannot open DSL file";
static const string ERROR_READ_DSL_FILE = "Error reading DSL file";
static const string ERROR_INCORRECT_DSL_FILE = "Incorrect DSL file";
static const string ERROR_DSL_ENCODING = "Cannot determine DSL file encoding";
static const string ERROR_DICTZIP = "Dictzip error";

struct DslLang {
  unsigned code;
  char codeStr[4];
};

}

#endif