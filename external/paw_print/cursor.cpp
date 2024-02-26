#include "./curses.h"

#include <iomanip>
#include <sstream>
#include <cstring>

#include "./paw_print.h"


namespace paw_print {


using std::stringstream;
using std::to_string;


Cursor::Cursor ()
:paw_print_(null),
 idx_(-1)
{
}


Cursor::Cursor (
    const shared_ptr<PawPrint> &paw_print,
    int idx
)
:paw_print_(paw_print),
 idx_(idx)
{
}


Cursor::Cursor (const Cursor &cursor)
:paw_print_(cursor.paw_print_),
 idx_(cursor.idx_)
{
}


DataType Cursor::type () const {
  if (paw_print_ == null || idx_ < 0)
    return PawPrint::Data::TYPE_NONE;

  if (paw_print_->isReference(idx_) == true)
    return paw_print_->getReference(idx_)->type();

  return paw_print_->type(idx_);
}


template<> bool Cursor::is<char       > () const { return type() == PawPrint::Data::TYPE_SINT_1B; }
template<> bool Cursor::is<byte       > () const { return type() == PawPrint::Data::TYPE_UINT_1B; }
template<> bool Cursor::is<bool       > () const { return type() == PawPrint::Data::TYPE_UINT_1B; }
template<> bool Cursor::is<short      > () const { return type() == PawPrint::Data::TYPE_SINT_2B; }
template<> bool Cursor::is<ushort     > () const { return type() == PawPrint::Data::TYPE_UINT_2B; }
template<> bool Cursor::is<int        > () const { return type() == PawPrint::Data::TYPE_SINT_4B; }
template<> bool Cursor::is<uint       > () const { return type() == PawPrint::Data::TYPE_UINT_4B; }
template<> bool Cursor::is<int64      > () const { return type() == PawPrint::Data::TYPE_SINT_8B; }
template<> bool Cursor::is<uint64     > () const { return type() == PawPrint::Data::TYPE_UINT_8B; }
template<> bool Cursor::is<float      > () const { return type() == PawPrint::Data::TYPE_REAL_4B; }
template<> bool Cursor::is<double     > () const { return type() == PawPrint::Data::TYPE_REAL_8B; }
template<> bool Cursor::is<string     > () const { return type() == PawPrint::Data::TYPE_STRING;  }
template<> bool Cursor::is<const char*> () const { return type() == PawPrint::Data::TYPE_STRING;  }


bool Cursor::isNumber () const {
  return type() >= PawPrint::Data::TYPE_SINT_1B && type() <= PawPrint::Data::TYPE_REAL_8B;
}


template<> bool Cursor::isConvertable<char  > () const { return isNumber(); }
template<> bool Cursor::isConvertable<byte  > () const { return isNumber(); }
template<> bool Cursor::isConvertable<bool  > () const { return isNumber(); }
template<> bool Cursor::isConvertable<short > () const { return isNumber(); }
template<> bool Cursor::isConvertable<ushort> () const { return isNumber(); }
template<> bool Cursor::isConvertable<int   > () const { return isNumber(); }
template<> bool Cursor::isConvertable<uint  > () const { return isNumber(); }
template<> bool Cursor::isConvertable<int64 > () const { return isNumber(); }
template<> bool Cursor::isConvertable<uint64> () const { return isNumber(); }
template<> bool Cursor::isConvertable<float > () const { return isNumber(); }
template<> bool Cursor::isConvertable<double> () const { return isNumber(); }


template<> bool Cursor::isConvertable<string> () const {
  return is<string>() || isNumber();
}


inline bool Cursor::isValid () const {
  if (paw_print_ == null || idx_ < 0)
    return false;

  if (paw_print_->isReference(idx_) == true)
    return paw_print_->getReference(idx_)->isValid();

  return true;
}


bool Cursor::isNull () const {
  return type() == PawPrint::Data::TYPE_NULL;
}


bool Cursor::isSequence () const {
  return type() == PawPrint::Data::TYPE_SEQUENCE;
}


bool Cursor::isMap () const {
  return type() == PawPrint::Data::TYPE_MAP;
}


bool Cursor::isKeyValuePair () const {
  return type() == PawPrint::Data::TYPE_KEY_VALUE_PAIR;
}


string Cursor::get (const char *default_value) const {
  return get<string>(default_value);
}


shared_ptr<Cursor> Cursor::getElem (int idx) const {
  if (paw_print_->isReference(idx_) == true)
    return paw_print_->getReference(idx_)->getElem(idx);

  if (isSequence() == false)
    return make_shared<Cursor>(paw_print_, -1);

  auto &data_idxs = paw_print_->getDataIdxsOfSequence(idx_);
  if (idx < 0 || idx >= data_idxs.size())
    return make_shared<Cursor>(paw_print_, -1);

  return make_shared<Cursor>(paw_print_, data_idxs[idx]);
}


shared_ptr<Cursor> Cursor::getElem (const char *key) const {
  if (paw_print_->isReference(idx_) == true)
    return paw_print_->getReference(idx_)->getElem(key);

  if (isMap() == false)
    return make_shared<Cursor>(paw_print_, -1);

  auto &sorted_data_idxs = paw_print_->getSortedDataIdxsOfMap(idx_);
  int pair_idx = paw_print_->findRawIdxOfValue(sorted_data_idxs, 0, sorted_data_idxs.size() - 1, key);
  if (pair_idx < 0)
    return make_shared<Cursor>(paw_print_, -1);

  auto value_idx = paw_print_->getValueRawIdxOfPair(pair_idx);
  return make_shared<Cursor>(paw_print_, value_idx);
}


shared_ptr<Cursor> Cursor::getElem (const string &key) const {
  return getElem(key.c_str());
}


shared_ptr<Cursor> Cursor::getKeyValuePair (int idx) const {
  if (paw_print_->isReference(idx_) == true)
    return paw_print_->getReference(idx_)->getKeyValuePair(idx);

  if (isMap() == false)
    return make_shared<Cursor>(paw_print_, -1);

  auto &data_idxs = paw_print_->getDataIdxsOfMap(idx_);
  return make_shared<Cursor>(paw_print_, data_idxs[idx]);
}


int Cursor::size () const {
  if (paw_print_->isReference(idx_) == true)
    return paw_print_->getReference(idx_)->size();

  if (isSequence() == true)
    return paw_print_->getDataIdxsOfSequence(idx_).size();
  else if (isMap() == true)
    return paw_print_->getDataIdxsOfMap(idx_).size();
  else
    return 1;
}


string Cursor::toString (int indent, int indent_inc, bool ignore_indent) const {
  if (paw_print_->isReference(idx_) == true)
    return paw_print_->getReference(idx_)->toString(indent, indent_inc, ignore_indent);

  stringstream ss;

  if (ignore_indent == false) {
    for (int i=0; i<indent; ++i)
      ss << " ";
  }

  switch (type()) {
    case PawPrint::Data::TYPE_NONE:
      ss << "NONE" << endl;
      break;

    case PawPrint::Data::TYPE_NULL:
      ss << "null" << endl;
      break;

    case PawPrint::Data::TYPE_SINT_1B:
    case PawPrint::Data::TYPE_UINT_1B:
    case PawPrint::Data::TYPE_SINT_2B:
    case PawPrint::Data::TYPE_UINT_2B:
    case PawPrint::Data::TYPE_SINT_4B:
    case PawPrint::Data::TYPE_UINT_4B:
    case PawPrint::Data::TYPE_SINT_8B:
    case PawPrint::Data::TYPE_UINT_8B:
      ss << get("0") << endl;
      break;

    case PawPrint::Data::TYPE_REAL_4B:
    case PawPrint::Data::TYPE_REAL_8B:
      ss << get("0.0") << endl;
      break;

    case PawPrint::Data::TYPE_STRING:
      ss << "\"" << get("") << "\"" << endl;
      break;

    case PawPrint::Data::TYPE_SEQUENCE:
      if (size() <= 0)
        ss << "[ ]" << endl;
      for (int i = 0; i < size(); ++i) {
        
        if (i != 0) {
          for (int i = 0; i<indent; ++i)
            ss << " ";
        }

        auto child = this->getElem(i);
        auto need_new_line = child->isSequence() || (child->isMap() && child->size() > 1);
        ss << "- ";
        if (need_new_line == true) {
          ss << "\n";
          for (int i = 0; i<indent + indent_inc; ++i)
            ss << " ";
        }
        ss << child->toString(indent + indent_inc, indent_inc, true);
      }
      break;

    case PawPrint::Data::TYPE_MAP:
      if (size() <= 0)
        ss << "{ }" << endl;
      for (int i=0; i<size(); ++i) {
        if (i != 0) {
          for (int i = 0; i<indent; ++i)
            ss << " ";
        }
        ss << getKeyOfPair(i) << " :" << endl;
        ss << getValueOfPair(i)->toString(indent + indent_inc, indent_inc);
      }
      break;

    default:
      // TODO err
      cout << "err: cannot cursor convert to string type \'"
          << to_string(type()) << "\'" << endl;
      return "";
  }

  return ss.str();
}


const string & Cursor::getName () const {
  if (paw_print_->isReference(idx_) == true)
    return paw_print_->getReference(idx_)->getName();

  return paw_print_->name();
}


int Cursor::getColumn () const {
  if (paw_print_->isReference(idx_) == true)
    return paw_print_->getReference(idx_)->getColumn();

  return paw_print_->getColumn(idx_);
}


int Cursor::getLine () const {
  if (paw_print_->isReference(idx_) == true)
    return paw_print_->getReference(idx_)->getLine();

  return paw_print_->getLine(idx_);
}


const char* Cursor::getKey () const {
  if (isValid() == false)
    return null;

  if (paw_print_->isReference(idx_) == true)
    return paw_print_->getReference(idx_)->getKey();

  if (isKeyValuePair() == true) {
    auto key_idx = paw_print_->getKeyRawIdxOfPair(idx_);
    return paw_print_->getStrValue(key_idx);
  }else if (isMap() == true && size() > 0) {
    return getKeyOfPair(0);
  }

  return null;
}


shared_ptr<Cursor> Cursor::getValue () const {
  if (isValid() == false)
    return make_shared<Cursor>(paw_print_, -1);

  if (paw_print_->isReference(idx_) == true)
    return paw_print_->getReference(idx_)->getValue();

  if (isKeyValuePair() == true) {
    auto value_idx = paw_print_->getValueRawIdxOfPair(idx_);
    return make_shared<Cursor>(paw_print_, value_idx);
  }else if (isMap() == true && size() > 0) {
    return getValueOfPair(0);
  }

  return make_shared<Cursor>(paw_print_, -1);
}


shared_ptr<Cursor> Cursor::findKeyValuePair (const char *key) const {
  if (paw_print_ == null)
    return make_shared<Cursor>(paw_print_, -1);

  if (paw_print_->isReference(idx_) == true)
    return paw_print_->getReference(idx_)->findKeyValuePair(key);

  if (isMap() == false)
    return make_shared<Cursor>(paw_print_, -1);

  for (int pi=0; pi<size(); ++pi) {
    if (strcmp(getKeyOfPair(pi), key) == 0)
      return getKeyValuePair(pi);
  }

  return make_shared<Cursor>(paw_print_, -1);
}




bool MergerCursor::isAvailableType (DataType type) {
  switch (type) {
    case PawPrint::Data::TYPE_SEQUENCE:
    case PawPrint::Data::TYPE_MAP:
    case PawPrint::Data::TYPE_KEY_VALUE_PAIR:
      return true;

    default:
      return false;
  }
}


MergerCursor::MergerCursor (
    unsigned short merge_level/*=1*/,
    bool need_merge_sequence/*=true*/,
    bool need_merge_map/*=true*/
)
:super(),
 size_(-1),
 merge_level_(merge_level),
 need_merge_sequence_(need_merge_sequence),
 need_merge_map_     (need_merge_map     )
{
  if (merge_level_ < 0)
    merge_level_ = 0;
}


MergerCursor::MergerCursor (
    const vector<shared_ptr<Cursor>> &cursor_stack,
    unsigned short merge_level/*=1*/,
    bool need_merge_sequence/*=true*/,
    bool need_merge_map/*=true*/
)
:super(),
 size_(-1),
 merge_level_(merge_level),
 need_merge_sequence_(need_merge_sequence),
 need_merge_map_     (need_merge_map     )
{
  if (merge_level_ < 0)
    merge_level_ = 0;

  // find type based on top
  DataType type = PawPrint::Data::TYPE_NONE;
  for (auto &c : cursor_stack) {
    auto c_type = c->type();
    if (isAvailableType(c_type) == true)
      type = c_type;
  }
  if (isAvailableType(type) == false)
    return;

  // push cursors
  cursor_stack_.clear();
  for (auto &c : cursor_stack) {
    if (c->type() == type)
      pushCursor(c);
  }
}


bool MergerCursor::pushCursor (const shared_ptr<Cursor> &cursor) {
  auto type_is_matched = type() == PawPrint::Data::TYPE_NONE || cursor->type() == type();
  if (type_is_matched == false)
    return false;

  cursor_stack_.push_back(cursor);
  size_ = -1;
  return true;
}


shared_ptr<Cursor> MergerCursor::popCursor () {
  if (cursor_stack_.size() <= 0)
    return make_shared<Cursor>();

  auto &c = cursor_stack_.back();
  cursor_stack_.pop_back();
  return c;
}


int MergerCursor::size () const {
  if (size_ >= 0)
    return size_;

  if (cursor_stack_.size() <= 0)
    return -1;


  //compute size
  switch (type()) {
    case PawPrint::Data::TYPE_SEQUENCE: {
      if (need_merge_sequence_ == false) {
        size_ = cursor_stack_.back()->size();
        break;
      }

      size_ = 0;
      for (auto &c : cursor_stack_) {
        if (c->size() > 0)
          size_ += c->size();
      }
      break;
    }

    case PawPrint::Data::TYPE_MAP: {
      _resetMapSizeAndKeyValuePairs();
      break;
    }

    case PawPrint::Data::TYPE_KEY_VALUE_PAIR:
      size_ = 1;
      break;

    default:
      cout << "err: unhandled type \'"
          << to_string(type()) << "\'" << endl;
      return -1;
  }

  return size_;
}


shared_ptr<Cursor> MergerCursor::getElem (int idx) const {
  if (cursor_stack_.size() <= 0)
    return make_shared<Cursor>();

  if (type() != PawPrint::Data::TYPE_SEQUENCE)
    return make_shared<Cursor>();

  if (need_merge_sequence_ == false)
    return cursor_stack_.back()->getElem(idx);

  for (auto &c : cursor_stack_) {
    if (idx >= c->size()) {
      idx -= c->size();
      continue;
    }

    // end of merge
    auto elem = c->getElem(idx);
    if (merge_level_ == 1)
      return elem;

    // case: not mergable
    if (MergerCursor::isAvailableType(elem->type()) == false)
      return elem;

    // keep going
    return shared_ptr<Cursor>(
        new MergerCursor(
          {elem},
          merge_level_-1,
          need_merge_sequence_,
          need_merge_map_
        )
    );
  }

  return make_shared<Cursor>();
}


shared_ptr<Cursor> MergerCursor::getElem (const char *key) const {
  if (cursor_stack_.size() <= 0)
    return make_shared<Cursor>();

  if (type() != PawPrint::Data::TYPE_MAP && type())
    return make_shared<Cursor>();

  if (need_merge_map_ == false)
    return cursor_stack_.back()->getElem(key);

  auto pair = findKeyValuePair(key);
  if (pair->isValid() == false)
    return pair;

  return pair->getValue();
}


shared_ptr<Cursor> MergerCursor::getElem (const string &key) const {
  return getElem(key.c_str());
}


shared_ptr<Cursor> MergerCursor::getKeyValuePair (int idx) const {
  if (cursor_stack_.size() <= 0)
    return make_shared<Cursor>();

  if (type() != PawPrint::Data::TYPE_MAP)
    return make_shared<Cursor>();

  if (need_merge_map_ == false)
    return cursor_stack_.back()->getKeyValuePair(idx);


  // make key_value_pairs_
  if (key_value_pairs_.size() <= 0)
    _resetMapSizeAndKeyValuePairs();


  // get pair on key_value_pairs_
  if (idx >= key_value_pairs_.size())
    return make_shared<Cursor>();

  return key_value_pairs_[idx];
}


void MergerCursor::_resetMapSizeAndKeyValuePairs () const {
  size_ = 0;
  key_value_pairs_.clear();
  
  if (cursor_stack_.size() <= 0)
    return;

  if (need_merge_map_ == false) {
    size_ = cursor_stack_.back()->size();
    return;
  }


  unordered_map<string, char> key_exists_map;
  for (int ci=cursor_stack_.size()-1; ci>=0; --ci) {
    auto &c = cursor_stack_[ci];
    for (int pi=0; pi<c->size(); ++pi) {
      const auto &pair = c->getKeyValuePair(pi);
      auto key = pair->getKey();
      if (key_exists_map.find(key) != key_exists_map.end())
        continue;
      key_exists_map[key] = 1;


      // end of merge
      if (merge_level_ == 1) {
        key_value_pairs_.push_back(pair);
        continue;
      }


      // if not sequence nor map
      auto v_type = pair->getValue()->type();
      if (v_type != PawPrint::Data::TYPE_SEQUENCE && v_type != PawPrint::Data::TYPE_MAP) {
        key_value_pairs_.push_back(pair);
        continue;
      }


      // keep going with MergerCursor
      vector<shared_ptr<Cursor>> mergee_list;
      for (auto &cur : cursor_stack_) {
        const auto &cur_pair = cur->findKeyValuePair(key);
        if (cur_pair->isValid() == true)
          mergee_list.push_back(cur_pair);
      }
      key_value_pairs_.push_back(
          shared_ptr<Cursor>(
            new MergerCursor(
              mergee_list,
              merge_level_,
              need_merge_sequence_,
              need_merge_map_
            )
          )
      );
    }
  }

  size_ = key_value_pairs_.size();
}


string MergerCursor::toString (int indent/*=0*/, int indent_inc/*=2*/, bool ignore_indent/*=false*/) const {
  if (cursor_stack_.size() <= 0)
    return "<empty MergerCursor>\n";

  stringstream ss;
  ss << "<MergerCursor>";

  if (ignore_indent == false) {
    ss << endl;
    for (int i=0; i<indent; ++i)
      ss << " ";
  }

  switch (type()) {
    case PawPrint::Data::TYPE_SEQUENCE:
      if (size() <= 0)
        ss << "[ ]" << endl;
      for (int i = 0; i < size(); ++i) {
        
        if (i != 0) {
          for (int i = 0; i<indent; ++i)
            ss << " ";
        }

        auto child = this->getElem(i);
        auto need_new_line = child->isSequence() || (child->isMap() && child->size() > 1);
        ss << "- ";
        if (need_new_line == true) {
          ss << "\n";
          for (int i = 0; i<indent + indent_inc; ++i)
            ss << " ";
        }
        ss << child->toString(indent + indent_inc, indent_inc, true);
      }
      break;

    case PawPrint::Data::TYPE_MAP:
      if (size() <= 0)
        ss << "{ }" << endl;
      for (int i=0; i<size(); ++i) {
        if (i != 0) {
          for (int i = 0; i<indent; ++i)
            ss << " ";
        }
        ss << getKeyOfPair(i) << " :" << endl;
        ss << getValueOfPair(i)->toString(indent + indent_inc, indent_inc);
      }
      break;
    default:
      // TODO err
      cout << "err: cannot MergerCursor convert to string type \'"
          << to_string(type()) << "\'" << endl;
      return "";
  }

  return ss.str();
}


const char* MergerCursor::getKey () const {
  if (cursor_stack_.size() <= 0)
    return null;

  return cursor_stack_.back()->getKey();
}


shared_ptr<Cursor> MergerCursor::getValue () const {
  if (cursor_stack_.size() <= 0)
    return make_shared<Cursor>();

  auto top_value = cursor_stack_.back()->getValue();
  auto top_value_type = top_value->type();
  if (MergerCursor::isAvailableType(top_value_type) == false)
    return top_value;

  if (need_merge_map_ == false || merge_level_ == 1)
    return top_value;


  // make merger cursor
  vector<shared_ptr<Cursor>> mergee_list;
  for (auto &c : cursor_stack_) {
    auto v = c->getValue();
    if (v->type() == top_value_type)
      mergee_list.push_back(v);
  }
  return shared_ptr<Cursor>(
      new MergerCursor(
        mergee_list,
        merge_level_-1,
        need_merge_sequence_,
        need_merge_map_
      )
  );
}


shared_ptr<Cursor> MergerCursor::findKeyValuePair (const char *key) const {
  // make key_value_pairs_
  if (key_value_pairs_.size() <= 0)
    _resetMapSizeAndKeyValuePairs();

  // find
  for (auto &p : key_value_pairs_) {
    if (strcmp(p->getKey(), key) == 0)
      return p;
  }

  return make_shared<Cursor>();
}


bool MergerCursor::isValid () const {
  if (cursor_stack_.size() <= 0)
    return false;

  return cursor_stack_.back()->isValid();
}


const string& MergerCursor::getName () const {
  if (name_.size() <= 0) {
    stringstream ss;
    ss << "<MergerCursor [";
    for (int ci=cursor_stack_.size()-1; ci>=0; --ci) {
      auto &c = cursor_stack_[ci];
      ss << "'" << c->getName() << "'";
      if (c != 0)
        ss << ",";
    }
    ss << "] >";
    name_ = ss.str();
  }

  return name_;
}


int MergerCursor::getColumn () const {
  if (cursor_stack_.size() <= 0)
    return -1;

  return cursor_stack_.back()->getColumn();
}


int MergerCursor::getLine () const {
  if (cursor_stack_.size() <= 0)
    return -1;

  return cursor_stack_.back()->getLine();
}





MapLoader::MapLoader () {
}


MapLoader::MapLoader (
  const vector<std::pair<string, LoaderFunc>> &cases
)
:cases_ (cases)
{
}


void MapLoader::load (const shared_ptr<Cursor> &cursor) {
  if (cursor->isMap() == false)
    return;

  for (int pi=0; pi<cursor->size(); ++pi) {
  auto pair = cursor->getKeyValuePair(pi);
  auto key   = pair->getKey  ();
  auto value = pair->getValue();

  for (auto &c : cases_) {
    if (c.first == key) {
      c.second(value);
      break;
    }
  }
  }
}



}
