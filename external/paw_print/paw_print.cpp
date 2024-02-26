#include "./paw_print.h"

#include <algorithm>
#include <cstring>
#include <iostream>

#include "./cursor.h"


namespace paw_print {


shared_ptr<Cursor> PawPrint::root (const shared_ptr<PawPrint> &paw_print) {
  if (paw_print == null || paw_print->raw_data_.size() <= 0)
    return null;

  return make_shared<Cursor>(paw_print, 0);
}

shared_ptr<Cursor> PawPrint::makeCursor (const shared_ptr<PawPrint> &paw_print, int idx) {
  if (paw_print == null || idx < 0)
    return null;

  if (paw_print->isReference(idx) == false)
    return make_shared<Cursor>(paw_print, idx);

  // for reference
  auto &c = paw_print->getReference(idx);
  if (c->isValid() == false) {
    cout << "err: reference (name: "<< paw_print->name_ << ", idx:" << idx << ") is invalid" << endl;
    return null;
  }

  return makeCursor(c->paw_print(), c->idx());
}



PawPrint::PawPrint (const string &name)
:name_(name),
 is_closed_(false),
 last_pushed_idx_(-1)
{
}

PawPrint::PawPrint (const string &name, const vector<byte> &raw_data)
:PawPrint(name)
{
  setRawData(raw_data);
}

PawPrint::PawPrint (const string &name, const shared_ptr<Cursor> &cursor)
:PawPrint(name)
{
  operator = (cursor);
}

PawPrint::PawPrint (const string &name, bool          value) :PawPrint(name) { pushBool  (value); }
PawPrint::PawPrint (const string &name, char          value) :PawPrint(name) { pushSint1B(value); } 
PawPrint::PawPrint (const string &name, byte          value) :PawPrint(name) { pushUint1B(value); } 
PawPrint::PawPrint (const string &name, short         value) :PawPrint(name) { pushSint2B(value); } 
PawPrint::PawPrint (const string &name, ushort        value) :PawPrint(name) { pushUint2B(value); } 
PawPrint::PawPrint (const string &name, int           value) :PawPrint(name) { pushSint4B(value); } 
PawPrint::PawPrint (const string &name, uint          value) :PawPrint(name) { pushUint4B(value); } 
PawPrint::PawPrint (const string &name, int64         value) :PawPrint(name) { pushSint8B(value); } 
PawPrint::PawPrint (const string &name, uint64        value) :PawPrint(name) { pushUint8B(value); } 
PawPrint::PawPrint (const string &name, float         value) :PawPrint(name) { pushReal4B(value); } 
PawPrint::PawPrint (const string &name, double        value) :PawPrint(name) { pushReal8B(value); } 
PawPrint::PawPrint (const string &name, const char   *value) :PawPrint(name) { pushString(value); }
PawPrint::PawPrint (const string &name, const string &value) :PawPrint(name) { pushString(value); }


const PawPrint& PawPrint::operator = (const shared_ptr<Cursor> &cursor) {
  auto cursor_idx = cursor->idx();
  auto data_size = cursor->paw_print()->dataSize(cursor_idx);

  raw_data_.resize(data_size);
  column_map_.clear();
  line_map_  .clear();
  last_pushed_idx_ = -1;

  // copy raw_data
  auto &cursor_raw_data = cursor->paw_print()->raw_data_;
  for (int di=0; di<raw_data_.size(); ++di) {
    raw_data_[di] = cursor_raw_data[di + cursor_idx];
  }

  // copy column_map
  auto &cursor_column_map = cursor->paw_print()->column_map_;
  for (auto &itr : cursor_column_map) {
    auto idx = itr.first - cursor_idx;
    if (idx < 0 || idx >= data_size)
      continue;

    column_map_[idx] = itr.second;
  }

  // copy line_map
  auto &cursor_line_map = cursor->paw_print()->line_map_;
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

DataType PawPrint::type (int idx) const {
  return getData<DataType>(idx);
}

bool PawPrint::isReference (int idx) const {
  if (idx < 0)
    return false;

  return _getRawData<DataType>(idx) == Data::TYPE_REFERENCE;
}

const shared_ptr<Cursor>& PawPrint::getReference (int idx) const {
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

    case Data::TYPE_SINT_1B: result += 1; break;
    case Data::TYPE_UINT_1B: result += 1; break;
    case Data::TYPE_SINT_2B: result += 2; break;
    case Data::TYPE_UINT_2B: result += 2; break;
    case Data::TYPE_SINT_4B: result += 4; break;
    case Data::TYPE_UINT_4B: result += 4; break;
    case Data::TYPE_SINT_8B: result += 8; break;
    case Data::TYPE_UINT_8B: result += 8; break;
    case Data::TYPE_REAL_4B: result += 4; break;
    case Data::TYPE_REAL_8B: result += 8; break;

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


int PawPrint::pushNull (uint column, uint line) {
  if (is_closed_ == true)
    return-1;

  last_pushed_idx_ = raw_data_.size();
  raw_data_.resize(last_pushed_idx_ + sizeof(DataType));
  *((DataType*)&raw_data_[last_pushed_idx_]) = Data::TYPE_NULL;

  if (column > 0)
    column_map_[last_pushed_idx_] = column;
  if (line > 0)
    line_map_[last_pushed_idx_] = line;

  return last_pushed_idx_;
}


template<typename T>
int __pushNumber (
    PawPrint const* paw_print,
    vector<byte>& raw_data,
    int& last_pushed_idx,
    unordered_map<int, uint>& column_map,
    unordered_map<int, uint>& line_map,
    DataType data_type,
    T value,
    uint column,
    uint line
) {
  if (paw_print->is_closed() == true)
    return -1;

  last_pushed_idx = raw_data.size();
  raw_data.resize(last_pushed_idx + sizeof(DataType) + sizeof(T));
  *((DataType*)&raw_data[last_pushed_idx                   ]) = data_type;
  *((T*       )&raw_data[last_pushed_idx + sizeof(DataType)]) = value;

  if (column > 0)
    column_map[last_pushed_idx] = column;
  if (line > 0)
    line_map[last_pushed_idx] = line;

  return last_pushed_idx;
}


int PawPrint::pushSint1B (char value, uint column, uint line) {
  return __pushNumber<char>(
      this,
      raw_data_,
      last_pushed_idx_,
      column_map_,
      line_map_,
      PawPrint::Data::TYPE_SINT_1B,
      value,
      column,
      line
  );
}


int PawPrint::pushUint1B (byte value, uint column, uint line) {
  return __pushNumber<byte>(
      this,
      raw_data_,
      last_pushed_idx_,
      column_map_,
      line_map_,
      PawPrint::Data::TYPE_UINT_1B,
      value,
      column,
      line
  );
} 


int PawPrint::pushSint2B (short value, uint column, uint line) {
  return __pushNumber<short>(
      this,
      raw_data_,
      last_pushed_idx_,
      column_map_,
      line_map_,
      PawPrint::Data::TYPE_SINT_2B,
      value,
      column,
      line
  );
} 


int PawPrint::pushUint2B (ushort value, uint column, uint line) {
  return __pushNumber<ushort>(
      this,
      raw_data_,
      last_pushed_idx_,
      column_map_,
      line_map_,
      PawPrint::Data::TYPE_UINT_2B,
      value,
      column,
      line
  );
} 


int PawPrint::pushSint4B (int value, uint column, uint line) {
  return __pushNumber<int>(
      this,
      raw_data_,
      last_pushed_idx_,
      column_map_,
      line_map_,
      PawPrint::Data::TYPE_SINT_4B,
      value,
      column,
      line
  );
} 


int PawPrint::pushUint4B (uint value, uint column, uint line) {
  return __pushNumber<uint>(
      this,
      raw_data_,
      last_pushed_idx_,
      column_map_,
      line_map_,
      PawPrint::Data::TYPE_UINT_4B,
      value,
      column,
      line
  );
} 


int PawPrint::pushSint8B (int64 value, uint column, uint line) {
  return __pushNumber<int64>(
      this,
      raw_data_,
      last_pushed_idx_,
      column_map_,
      line_map_,
      PawPrint::Data::TYPE_SINT_8B,
      value,
      column,
      line
  );
} 


int PawPrint::pushUint8B (uint64 value, uint column, uint line) {
  return __pushNumber<uint64>(
      this,
      raw_data_,
      last_pushed_idx_,
      column_map_,
      line_map_,
      PawPrint::Data::TYPE_UINT_8B,
      value,
      column,
      line
  );
} 


int PawPrint::pushReal4B (float value, uint column, uint line) {
  return __pushNumber<float>(
      this,
      raw_data_,
      last_pushed_idx_,
      column_map_,
      line_map_,
      PawPrint::Data::TYPE_REAL_4B,
      value,
      column,
      line
  );
} 


int PawPrint::pushReal8B (double value, uint column, uint line) {
  return __pushNumber<double>(
      this,
      raw_data_,
      last_pushed_idx_,
      column_map_,
      line_map_,
      PawPrint::Data::TYPE_REAL_8B,
      value,
      column,
      line
  );
} 


int PawPrint::pushBool (bool value, uint column, uint line) {
  return pushUint1B(value, column, line);
}


int PawPrint::pushString (const char *value, uint column, uint line) {
  if (is_closed_ == true)
    return -1;

  last_pushed_idx_ = raw_data_.size();
  int str_count = strlen(value) + 1;
  raw_data_.resize(
      last_pushed_idx_
        + sizeof(DataType)
        + sizeof(Data::StrSizeType)
        + sizeof(const char) * str_count
  );
  *((DataType*         )&raw_data_[last_pushed_idx_                   ]) = Data::TYPE_STRING;
  *((Data::StrSizeType*)&raw_data_[last_pushed_idx_ + sizeof(DataType)]) = str_count;
  auto p = (const char*)&raw_data_[
      last_pushed_idx_ + sizeof(DataType) + sizeof(Data::StrSizeType)
  ];
  memcpy((void*)p, (void*)value, sizeof(const char) * str_count);

  if (column > 0)
    column_map_[last_pushed_idx_] = column;
  if (line > 0)
    line_map_[last_pushed_idx_] = line;

  return last_pushed_idx_;
}


int PawPrint::pushReference (const shared_ptr<Cursor> &cursor, uint column, uint line) {
  if (is_closed_ == true)
    return -1;

  last_pushed_idx_ = raw_data_.size();
  raw_data_.resize(
      last_pushed_idx_
        + sizeof(DataType)
        + sizeof(Data::ReferenceIdxType)
  );
  *((DataType*              )&raw_data_[last_pushed_idx_                   ]) = Data::TYPE_REFERENCE;
  *((Data::ReferenceIdxType*)&raw_data_[last_pushed_idx_ + sizeof(DataType)]) = references_.size();

  references_.push_back(cursor);

  if (column > 0)
    column_map_[last_pushed_idx_] = column;
  if (line > 0)
    line_map_[last_pushed_idx_] = line;

  return last_pushed_idx_;
}


int PawPrint::beginSequence (uint column, uint line) {
  if (is_closed_ == true)
    return -1;

  last_pushed_idx_ = raw_data_.size();
  raw_data_.resize(last_pushed_idx_ + sizeof(DataType));
  *((DataType*)&raw_data_[last_pushed_idx_]) = Data::TYPE_SEQUENCE_START;

  if (column > 0)
    column_map_[last_pushed_idx_] = column;
  if (line > 0)
    line_map_[last_pushed_idx_] = line;

  return last_pushed_idx_;
}


int PawPrint::endSequence (uint column, uint line) {
  if (is_closed_ == true)
    return -1;

  last_pushed_idx_ = raw_data_.size();
  raw_data_.resize(last_pushed_idx_ + sizeof(DataType));
  *((DataType*)&raw_data_[last_pushed_idx_]) = Data::TYPE_SEQUENCE_END;

  if (column > 0)
    column_map_[last_pushed_idx_] = column;
  if (line > 0)
    line_map_[last_pushed_idx_] = line;

  return last_pushed_idx_;
}


int PawPrint::pushKeyValuePair (uint column, uint line) {
  if (is_closed_ == true)
    return -1;

  last_pushed_idx_ = raw_data_.size();
  raw_data_.resize(last_pushed_idx_ + sizeof(DataType));
  *((DataType*)&raw_data_[last_pushed_idx_]) = Data::TYPE_KEY_VALUE_PAIR;

  if (column > 0)
    column_map_[last_pushed_idx_] = column;
  if (line > 0)
    line_map_[last_pushed_idx_] = line;

  return last_pushed_idx_;
}


int PawPrint::pushKey (const char *value, uint column, uint line) {
  if (is_closed_ == true)
    return -1;

  auto idx = pushKeyValuePair(column, line);

  pushString(value, column, line);

  return idx;
}


int PawPrint::beginMap (uint column, uint line) {
  if (is_closed_ == true)
    return -1;

  last_pushed_idx_ = raw_data_.size();
  raw_data_.resize(last_pushed_idx_ + sizeof(DataType));
  *((DataType*)&raw_data_[last_pushed_idx_]) = Data::TYPE_MAP_START;

  if (column > 0)
    column_map_[last_pushed_idx_] = column;
  if (line > 0)
    line_map_[last_pushed_idx_] = line;

  return last_pushed_idx_;
}


int PawPrint::endMap (uint column, uint line) {
  if (is_closed_ == true)
    return -1;

  last_pushed_idx_ = raw_data_.size();
  raw_data_.resize(last_pushed_idx_ + sizeof(DataType));
  *((DataType*)&raw_data_[last_pushed_idx_]) = Data::TYPE_MAP_END;

  if (column > 0)
    column_map_[last_pushed_idx_] = column;
  if (line > 0)
    line_map_[last_pushed_idx_] = line;

  return last_pushed_idx_;
}


void PawPrint::setRawData (const vector<byte> &raw_data) {
  raw_data_ = raw_data;
}


uint PawPrint::getColumn (int idx) const {
  if (column_map_.find(idx) == column_map_.end())
    return 0;

  return column_map_.at(idx);
}


uint PawPrint::getLine (int idx) const {
  if (line_map_.find(idx) == line_map_.end())
    return 0;

  return line_map_.at(idx);
}


uint PawPrint::findMaxLine () const {
  uint max_line = 0;

  for (auto &itr : line_map_) {
    if (itr.second > max_line)
      max_line = itr.second;
  }

  return max_line;
}



}
