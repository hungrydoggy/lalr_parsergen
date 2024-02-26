#ifndef EXTERNAL_PAW_PRINT_SRC_PAW_PRINT
#define EXTERNAL_PAW_PRINT_SRC_PAW_PRINT

#include <iostream>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

#include "../src/token.h"
#include "../src/node.h"

#include "./defines.h"


namespace paw_print {


using std::cout;
using std::endl;
using std::stack;
using std::string;
using std::unordered_map;
using std::vector;

using namespace parse_table;


class Cursor;


class PAW_PRINT_API TokenType {
public:
  enum Type {
    END_OF_FILE,
    INDENT,
    DEDENT,
    BOOL,
    INT,
    DOUBLE,
    STRING,
    COLON,
    COMMA,
    DASH,
    SHARP,
    SQUARE_OPEN,
    SQUARE_CLOSE,
    CURLY_OPEN,
    CURLY_CLOSE,
    NEW_LINE,
  };
};


class PAW_PRINT_API PawPrint {
public:
  class PAW_PRINT_API Data {
  public:
    using StrSizeType = unsigned short;
    using ReferenceIdxType = uint;


    static const DataType TYPE_NONE = 0xff;

    static const DataType TYPE_NULL = 0;

    static const DataType TYPE_SINT_1B =  1;  // char
    static const DataType TYPE_UINT_1B =  2;  // byte
    static const DataType TYPE_SINT_2B =  3;  // short
    static const DataType TYPE_UINT_2B =  4;  // ushort
    static const DataType TYPE_SINT_4B =  5;  // int
    static const DataType TYPE_UINT_4B =  6;  // uint
    static const DataType TYPE_SINT_8B =  7;  // int64
    static const DataType TYPE_UINT_8B =  8;  // uint64
    static const DataType TYPE_REAL_4B =  9;  // float
    static const DataType TYPE_REAL_8B = 10;  // double
    static const DataType TYPE_STRING  = 11;

    static const DataType TYPE_SEQUENCE       = 12;
    static const DataType TYPE_SEQUENCE_START = 12;
    static const DataType TYPE_SEQUENCE_END   = 13;

    static const DataType TYPE_MAP       = 14;
    static const DataType TYPE_MAP_START = 14;
    static const DataType TYPE_MAP_END   = 15;

    static const DataType TYPE_KEY_VALUE_PAIR = 16;

    static const DataType TYPE_REFERENCE = 17;
  };


public:
  static shared_ptr<Cursor> root (const shared_ptr<PawPrint> &paw_print);

  static shared_ptr<Cursor> makeCursor (const shared_ptr<PawPrint> &paw_print, int idx);


public:
  PawPrint (const string &name);

  PawPrint (const string &name, const vector<byte> &raw_data);
  PawPrint (const string &name, const shared_ptr<Cursor> &cursor);

  PawPrint (const string &name, bool   value);
  PawPrint (const string &name, char   value); 
  PawPrint (const string &name, byte   value); 
  PawPrint (const string &name, short  value); 
  PawPrint (const string &name, ushort value); 
  PawPrint (const string &name, int    value); 
  PawPrint (const string &name, uint   value); 
  PawPrint (const string &name, int64  value); 
  PawPrint (const string &name, uint64 value); 
  PawPrint (const string &name, float  value); 
  PawPrint (const string &name, double value); 

  PawPrint (const string &name, const char   *value);
  PawPrint (const string &name, const string &value);

  PawPrint (const string &name, const vector<int   > &value);
  PawPrint (const string &name, const vector<double> &value);
  PawPrint (const string &name, const vector<string> &value);

  ~PawPrint ();


  PAW_GETTER_SETTER(const string&, name)
  PAW_GETTER_SETTER(int, last_pushed_idx)

  PAW_GETTER(bool, is_closed)


  inline vector<byte>& raw_data () {
    return raw_data_;
  }

  DataType type (int idx) const;
  bool isReference (int idx) const;


  const PawPrint& operator = (const shared_ptr<Cursor> &cursor);
  int dataSize (int idx) const;

  void setRawData (const vector<byte> &raw_data);

  uint getColumn (int idx) const;
  uint getLine (int idx) const;
  uint findMaxLine () const;


  // write
  int pushSint1B (char   value, uint column=0, uint line=0); 
  int pushUint1B (byte   value, uint column=0, uint line=0); 
  int pushSint2B (short  value, uint column=0, uint line=0); 
  int pushUint2B (ushort value, uint column=0, uint line=0); 
  int pushSint4B (int    value, uint column=0, uint line=0); 
  int pushUint4B (uint   value, uint column=0, uint line=0); 
  int pushSint8B (int64  value, uint column=0, uint line=0); 
  int pushUint8B (uint64 value, uint column=0, uint line=0); 
  int pushReal4B (float  value, uint column=0, uint line=0); 
  int pushReal8B (double value, uint column=0, uint line=0); 

  int pushBool   (bool   value, uint column=0, uint line=0); 

  int pushString (const char *value, uint column=0, uint line=0); 
  inline int pushString (const string &value, uint column=0, uint line=0) {
    return pushString(value.c_str(), column, line);
  }

  int pushNull   (uint column=0, uint line=0); 

  int pushReference (const shared_ptr<Cursor> &cursor, uint column=0, uint line=0);

  int beginSequence (uint column=0, uint line=0);
  int endSequence   (uint column=0, uint line=0);

  int beginMap (uint column=0, uint line=0);
  int endMap   (uint column=0, uint line=0);

  int pushKeyValuePair (uint column=0, uint line=0);

  int pushKey (const char *value, uint column=0, uint line=0);
  inline int pushKey (const string &value, uint column=0, uint line=0) {
    return pushKey(value.c_str(), column, line);
  }


  // read
  Data::StrSizeType getStrSize (int idx) const;
  const char* getStrValue (int idx) const;

  int getKeyRawIdxOfPair   (int pair_idx) const;
  int getValueRawIdxOfPair (int pair_idx) const;

  template <class T>
  const T& getData (int idx) const {
    return _getRawData<T>(idx);
  }

  const shared_ptr<Cursor>& getReference (int idx) const;

  const vector<int>& getDataIdxsOfSequence (int sequence_idx) const;
  const vector<int>& getDataIdxsOfMap     (int map_idx) const;
  const vector<int>& getSortedDataIdxsOfMap (int map_idx) const;

  int findRawIdxOfValue (
      const vector<int> &map_datas,
      int first,
      int last,
      const char *key) const;


private:
  string name_;
  vector<byte> raw_data_;
  mutable unordered_map<int, vector<int>> data_idxs_of_sequence_map_;
  mutable unordered_map<int, vector<int>> data_idxs_of_map_map_;
  mutable unordered_map<int, vector<int>> sorted_data_idxs_of_map_map_;
  bool is_closed_;
  int last_pushed_idx_;

  vector<shared_ptr<Cursor>> references_;

  stack<int> curly_open_idx_stack_;
  stack<int> square_open_idx_stack_;
  unordered_map<int, uint> column_map_;
  unordered_map<int, uint> line_map_;


  template <class T>
  const T& _getRawData (int idx) const {
    return *((T*)&raw_data_[idx]);
  }
};

}


// for paw_print user
#include "./cursor.h"

#include "./undefines.h"

#endif
