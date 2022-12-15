#include "./paw_print.h"

#include <algorithm>
#include <cstring>
#include <iostream>


namespace paw_print {


PawPrint::PawPrint (const string &name)
:name_(name),
 is_closed_(false)
{
}

PawPrint::PawPrint (const string &name, const vector<unsigned char> &raw_data)
:PawPrint(name)
{
  setRawData(raw_data);
}

PawPrint::PawPrint (const string &name, const Cursor &cursor)
:PawPrint(name)
{
  operator = (cursor);
}

PawPrint::PawPrint (const string &name, bool          value) :PawPrint(name) { pushBool  (value); }
PawPrint::PawPrint (const string &name, int           value) :PawPrint(name) { pushInt   (value); }
PawPrint::PawPrint (const string &name, float         value) :PawPrint(name) { pushDouble(value); }
PawPrint::PawPrint (const string &name, double        value) :PawPrint(name) { pushDouble(value); }
PawPrint::PawPrint (const string &name, const char   *value) :PawPrint(name) { pushString(value); }
PawPrint::PawPrint (const string &name, const string &value) :PawPrint(name) { pushString(value); }


#define PAW_PRINT_VECTOR_CONSTRUCTOR(TYPE, PUSH_TYPE) \
PawPrint::PawPrint (const string &name, const vector<TYPE> &value) \
:PawPrint(name) \
{ \
  beginSequence(); \
    for (auto v : value) { \
      push##PUSH_TYPE(v); \
    } \
  endSequence(); \
}

PAW_PRINT_VECTOR_CONSTRUCTOR(int   , Int   )
PAW_PRINT_VECTOR_CONSTRUCTOR(double, Double)
PAW_PRINT_VECTOR_CONSTRUCTOR(string, String)
#undef PAW_PRINT_VECTOR_CONSTRUCTOR


const PawPrint& PawPrint::operator = (const Cursor &cursor) {
  auto cursor_idx = cursor.idx();
  auto data_size = cursor.paw_print()->dataSize(cursor_idx);

  raw_data_.resize(data_size);
  column_map_.clear();
  line_map_  .clear();

  // copy raw_data
  auto &cursor_raw_data = cursor.paw_print()->raw_data_;
  for (int di=0; di<raw_data_.size(); ++di) {
    raw_data_[di] = cursor_raw_data[di + cursor_idx];
  }

  // copy column_map
  auto &cursor_column_map = cursor.paw_print()->column_map_;
  for (auto &itr : cursor_column_map) {
    auto idx = itr.first - cursor_idx;
    if (idx < 0 || idx >= data_size)
      continue;

    column_map_[idx] = itr.second;
  }

  // copy line_map
  auto &cursor_line_map = cursor.paw_print()->line_map_;
  for (auto &itr : cursor_line_map) {
    auto idx = itr.first - cursor_idx;
    if (idx < 0 || idx >= data_size)
      continue;

    line_map_[idx] = itr.second;
  }

  return *this;
}

PawPrint::~PawPrint () {
}

PawPrint::Cursor PawPrint::root () const {
  if (raw_data_.size() <= 0)
    return Cursor(this, -1);

  return Cursor(this, 0);
}

PawPrint::Cursor PawPrint::makeCursor (int idx) const {
  if (idx < 0)
    return Cursor(this, -1);

  if (isReference(idx) == false)
    return Cursor(this, idx);

  // for reference
  auto &c = _getReference(idx);
  if (c.isValid() == false) {
    cout << "err: reference (name: "<< name_ << ", idx:" << idx << ") is invalid" << endl;
    return Cursor(this, -1);
  }

  return c.paw_print()->makeCursor(c.idx());
}

DataType PawPrint::type (int idx) const {
  return getData<DataType>(idx);
}

bool PawPrint::isReference (int idx) const {
  if (idx < 0)
    return false;

  return _getRawData<DataType>(idx) == Data::TYPE_REFERENCE;
}

const PawPrint::Cursor& PawPrint::_getReference (int idx) const {
  auto ri = _getRawData<PawPrint::Data::ReferenceIdxType>(idx + sizeof(DataType));
  return references_[ri];
}

PawPrint::Data::StrSizeType PawPrint::getStrSize (int idx) const {
  return _getRawData<PawPrint::Data::StrSizeType>(idx + sizeof(DataType));
}

const char* PawPrint::getStrValue (int idx) const {
  return (const char*)&raw_data_[idx + sizeof(DataType) + sizeof(PawPrint::Data::StrSizeType)];
}

int PawPrint::getKeyRawIdxOfPair (int pair_idx) const {
  return pair_idx + sizeof(DataType);
}

int PawPrint::getValueRawIdxOfPair (int pair_idx) const {
  auto key_idx = getKeyRawIdxOfPair(pair_idx);
  return key_idx + dataSize(key_idx);
}

int PawPrint::dataSize (int idx) const {
  int result = sizeof(DataType);

  auto t = type(idx);
  switch (t) {
    case Data::TYPE_NULL: break;

    case Data::TYPE_BOOL  : result += sizeof(char  ); break;
    case Data::TYPE_INT   : result += sizeof(int   ); break;
    case Data::TYPE_DOUBLE: result += sizeof(double); break;

    case Data::TYPE_STRING:
      result += sizeof(Data::StrSizeType) + sizeof(const char) * getStrSize(idx);
      break;

    case Data::TYPE_SEQUENCE_START:
      for (int raw_idx : getDataIdxsOfSequence(idx))
        result += dataSize(raw_idx);
      result += sizeof(DataType);
      break;

    case Data::TYPE_MAP_START:
      for (int raw_idx : getDataIdxsOfMap(idx))
        result += dataSize(raw_idx);
      result += sizeof(DataType);
      break;

    case Data::TYPE_KEY_VALUE_PAIR:
      result += dataSize(idx + result); // key size
      result += dataSize(idx + result); // value size
      break;

    case Data::TYPE_REFERENCE:
      result += sizeof(Data::ReferenceIdxType);
      break;

    default: break;
  }

  return result;
}

const vector<int>& PawPrint::getDataIdxsOfSequence (int sequence_idx) const {
  if (data_idxs_of_sequence_map_.find(sequence_idx) != data_idxs_of_sequence_map_.end())
    return data_idxs_of_sequence_map_[sequence_idx];

  auto &result = data_idxs_of_sequence_map_[sequence_idx];
  auto idx = sequence_idx + sizeof(DataType);
  while (type(idx) != Data::TYPE_SEQUENCE_END) {
    result.push_back(idx);

    idx += dataSize(idx);
  }
  
  return result;
}

class SortFuncForKey {
public:
  SortFuncForKey (const PawPrint &paw_print)
  :paw_print_(paw_print)
  {
  }

  bool operator () (int a_idx, int b_idx) {
    auto a_key_idx = paw_print_.getKeyRawIdxOfPair(a_idx);
    auto b_key_idx = paw_print_.getKeyRawIdxOfPair(b_idx);
    return strcmp(paw_print_.getStrValue(a_key_idx), paw_print_.getStrValue(b_key_idx)) < 0;
  }

private:
  const PawPrint &paw_print_;
};


static void _makeDataIdxsOfMap (
    const PawPrint *paw,
    int map_idx,
    unordered_map<int, vector<int>> &data_idxs_of_map_map,
    unordered_map<int, vector<int>> &sorted_data_idxs_of_map_map
) {

  // make datas
  auto &result   = data_idxs_of_map_map[map_idx];
  auto &sorted_res = sorted_data_idxs_of_map_map[map_idx];
  auto idx = map_idx + sizeof(DataType);
  while (paw->type(idx) != PawPrint::Data::TYPE_MAP_END) {
    result  .push_back(idx);
    sorted_res.push_back(idx);

    idx += paw->dataSize(idx);
  }

  std::sort(sorted_res.begin(), sorted_res.end(), SortFuncForKey(*paw));
}

const vector<int>& PawPrint::getDataIdxsOfMap (int map_idx) const {
  if (data_idxs_of_map_map_.find(map_idx) == data_idxs_of_map_map_.end())
    _makeDataIdxsOfMap(this, map_idx, data_idxs_of_map_map_, sorted_data_idxs_of_map_map_);

  return data_idxs_of_map_map_[map_idx];
}

const vector<int>& PawPrint::getSortedDataIdxsOfMap(int map_idx) const {
  if (sorted_data_idxs_of_map_map_.find(map_idx) == sorted_data_idxs_of_map_map_.end())
    _makeDataIdxsOfMap(this, map_idx, data_idxs_of_map_map_, sorted_data_idxs_of_map_map_);

  return sorted_data_idxs_of_map_map_[map_idx];
}

int PawPrint::findRawIdxOfValue (
    const vector<int> &sorted_map_datas,
    int first,
    int last,
    const char *key
) const {

  if (first > last)
    return -1;

  int mid = (first + last) / 2;
  auto mid_pair_idx = sorted_map_datas[mid];
  auto mid_key_idx = getKeyRawIdxOfPair(mid_pair_idx);
  auto mid_key = getStrValue(mid_key_idx);

  auto cmp_res = strcmp(key, mid_key);
  if (cmp_res < 0)
    return findRawIdxOfValue(sorted_map_datas, first, mid - 1, key);
  else if (cmp_res > 0)
    return findRawIdxOfValue(sorted_map_datas, mid + 1, last, key);
  else
    return mid_pair_idx;
}

int PawPrint::pushNull (int column, int line) {
  if (is_closed_ == true)
    return-1;

  int old_size = raw_data_.size();
  raw_data_.resize(old_size + sizeof(DataType));
  *((DataType*)&raw_data_[old_size]) = Data::TYPE_NULL;

  if (column >= 0)
    column_map_[old_size] = (unsigned short)column;
  if (line >= 0)
    line_map_[old_size] = (unsigned short)line;

  return old_size;
}

int PawPrint::pushBool (bool value, int column, int line) {
  if (is_closed_ == true)
    return -1;

  int old_size = raw_data_.size();
  raw_data_.resize(old_size + sizeof(DataType) + sizeof(char));
  *((DataType*)&raw_data_[old_size                   ]) = Data::TYPE_BOOL;
  *((char*    )&raw_data_[old_size + sizeof(DataType)]) = (char)value;

  if (column >= 0)
    column_map_[old_size] = (unsigned short)column;
  if (line >= 0)
    line_map_[old_size] = (unsigned short)line;

  return old_size;
}

int PawPrint::pushInt (int value, int column, int line) {
  if (is_closed_ == true)
    return -1;

  int old_size = raw_data_.size();
  raw_data_.resize(old_size + sizeof(DataType) + sizeof(int));
  *((DataType*)&raw_data_[old_size                   ]) = Data::TYPE_INT;
  *((int*     )&raw_data_[old_size + sizeof(DataType)]) = value;

  if (column >= 0)
    column_map_[old_size] = (unsigned short)column;
  if (line >= 0)
    line_map_[old_size] = (unsigned short)line;

  return old_size;
}

int PawPrint::pushDouble (double value, int column, int line) {
  if (is_closed_ == true)
    return -1;

  int old_size = raw_data_.size();
  raw_data_.resize(old_size + sizeof(DataType) + sizeof(double));
  *((DataType*)&raw_data_[old_size                   ]) = Data::TYPE_DOUBLE;
  *((double*  )&raw_data_[old_size + sizeof(DataType)]) = value;

  if (column >= 0)
    column_map_[old_size] = (unsigned short)column;
  if (line >= 0)
    line_map_[old_size] = (unsigned short)line;

  return old_size;
}

int PawPrint::pushString (const char *value, int column, int line) {
  if (is_closed_ == true)
    return -1;

  int old_size = raw_data_.size();
  int str_count = strlen(value) + 1;
  raw_data_.resize(
      old_size
        + sizeof(DataType)
        + sizeof(Data::StrSizeType)
        + sizeof(const char) * str_count
  );
  *((DataType*         )&raw_data_[old_size                   ]) = Data::TYPE_STRING;
  *((Data::StrSizeType*)&raw_data_[old_size + sizeof(DataType)]) = str_count;
  auto p = (const char*)&raw_data_[
      old_size + sizeof(DataType) + sizeof(Data::StrSizeType)
  ];
  memcpy((void*)p, (void*)value, sizeof(const char) * str_count);

  if (column >= 0)
    column_map_[old_size] = (unsigned short)column;
  if (line >= 0)
    line_map_[old_size] = (unsigned short)line;

  return old_size;
}

int PawPrint::pushReference (const Cursor &cursor, int column, int line) {
  if (is_closed_ == true)
    return -1;

  int old_size = raw_data_.size();
  raw_data_.resize(
      old_size
        + sizeof(DataType)
        + sizeof(Data::ReferenceIdxType)
  );
  *((DataType*              )&raw_data_[old_size                   ]) = Data::TYPE_REFERENCE;
  *((Data::ReferenceIdxType*)&raw_data_[old_size + sizeof(DataType)]) = references_.size();

  references_.push_back(cursor);

  if (column >= 0)
    column_map_[old_size] = (unsigned short)column;
  if (line >= 0)
    line_map_[old_size] = (unsigned short)line;

  return old_size;
}

int PawPrint::beginSequence (int column, int line) {
  if (is_closed_ == true)
    return -1;

  int old_size = raw_data_.size();
  raw_data_.resize(old_size + sizeof(DataType));
  *((DataType*)&raw_data_[old_size]) = Data::TYPE_SEQUENCE_START;

  if (column >= 0)
    column_map_[old_size] = (unsigned short)column;
  if (line >= 0)
    line_map_[old_size] = (unsigned short)line;

  return old_size;
}

int PawPrint::endSequence (int column, int line) {
  if (is_closed_ == true)
    return -1;

  int old_size = raw_data_.size();
  raw_data_.resize(old_size + sizeof(DataType));
  *((DataType*)&raw_data_[old_size]) = Data::TYPE_SEQUENCE_END;

  if (column >= 0)
    column_map_[old_size] = (unsigned short)column;
  if (line >= 0)
    line_map_[old_size] = (unsigned short)line;

  return old_size;
}

int PawPrint::pushKeyValuePair (int column, int line) {
  if (is_closed_ == true)
    return -1;

  int old_size = raw_data_.size();
  raw_data_.resize(old_size + sizeof(DataType));
  *((DataType*)&raw_data_[old_size]) = Data::TYPE_KEY_VALUE_PAIR;

  if (column >= 0)
    column_map_[old_size] = (unsigned short)column;
  if (line >= 0)
    line_map_[old_size] = (unsigned short)line;

  return old_size;
}

int PawPrint::pushKey (const char *value, int column, int line) {
  if (is_closed_ == true)
    return -1;

  auto idx = pushKeyValuePair(column, line);

  pushString(value, column, line);

  return idx;
}

int PawPrint::beginMap (int column, int line) {
  if (is_closed_ == true)
    return -1;

  int old_size = raw_data_.size();
  raw_data_.resize(old_size + sizeof(DataType));
  *((DataType*)&raw_data_[old_size]) = Data::TYPE_MAP_START;

  if (column >= 0)
    column_map_[old_size] = (unsigned short)column;
  if (line >= 0)
    line_map_[old_size] = (unsigned short)line;

  return old_size;
}

int PawPrint::endMap (int column, int line) {
  if (is_closed_ == true)
    return -1;

  int old_size = raw_data_.size();
  raw_data_.resize(old_size + sizeof(DataType));
  *((DataType*)&raw_data_[old_size]) = Data::TYPE_MAP_END;

  if (column >= 0)
    column_map_[old_size] = (unsigned short)column;
  if (line >= 0)
    line_map_[old_size] = (unsigned short)line;

  return old_size;
}

void PawPrint::setRawData (const vector<unsigned char> &raw_data) {
  raw_data_ = raw_data;
}

int PawPrint::getColumn (int idx) const {
  if (column_map_.find(idx) == column_map_.end())
    return -1;

  return column_map_.at(idx);
}

int PawPrint::getLine (int idx) const {
  if (line_map_.find(idx) == line_map_.end())
    return -1;

  return line_map_.at(idx);
}

}
