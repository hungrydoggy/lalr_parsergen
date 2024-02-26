#ifndef EXTERNAL_PAW_PRINT_SRC_CURSOR
#define EXTERNAL_PAW_PRINT_SRC_CURSOR

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#include "./defines.h"
#include "./paw_print.h"


namespace paw_print {


using std::function;
using std::shared_ptr;
using std::string;
using std::vector;


class PAW_PRINT_API Cursor {
public:
  Cursor ();
  Cursor (
      const shared_ptr<PawPrint> &paw_print,
      int idx
  );
  Cursor (const Cursor &cursor);
  virtual ~Cursor () {}


  virtual const shared_ptr<PawPrint>& paw_print () const { return paw_print_; }

  virtual int idx () const { return idx_; }
  virtual DataType type () const;
  virtual int size () const;


  template <class T>
  bool is () const { return false; }

  bool isNumber () const;

  template <class T>
  bool isConvertable () const { return false; }

  bool isSequence () const;
  bool isMap () const;
  bool isKeyValuePair () const;

  bool isNull () const;

  virtual bool isValid () const;

  string get (char const* default_value) const;

  // for number or string
  template <typename T>
  /*T*/std::enable_if_t<std::is_arithmetic_v<T> || std::is_same_v<std::string, T>, T>
  get (T const& default_value) const {
    if (isValid() == false)
      return default_value;

    if (paw_print_->isReference(idx_) == true)
      return paw_print_->getReference(idx_)->get<T>(default_value);

    if (isConvertable<T>() == false)
      return default_value;

    return _getConvertedData<T>(default_value);
  }

  // for non-number and non-string
  template <typename T>
  /*T*/std::enable_if_t<!std::is_arithmetic_v<T> && !std::is_same_v<std::string, T>, T>
  get (T const& default_value) const {
    return default_value;
  }

  virtual shared_ptr<Cursor> getElem (int idx) const;
  virtual shared_ptr<Cursor> getElem (const char *key) const;
  virtual shared_ptr<Cursor> getElem (const string &key) const;

  virtual shared_ptr<Cursor> getKeyValuePair (int idx) const;

  virtual string toString (int indent=0, int indent_inc=2, bool ignore_indent=false) const;

  virtual const char* getKey () const;
  virtual shared_ptr<Cursor> getValue () const;
  inline const char* getKeyOfPair(int idx) const {
    auto c = getKeyValuePair(idx);
    return c->getKey();
  }
  inline shared_ptr<Cursor> getValueOfPair(int idx) const {
    auto c = getKeyValuePair(idx);
    return c->getValue();
  }

  virtual shared_ptr<Cursor> findKeyValuePair (const char *key) const;

  virtual const string & getName () const;
  virtual int getColumn () const;
  virtual int getLine () const;


private: // vars
  shared_ptr<PawPrint> paw_print_;
  int idx_;

  
private: // methods
  // for number
  template <class T>
  /*T*/std::enable_if_t<std::is_arithmetic_v<T>, T>
  _getConvertedData (T const& default_value) const {
    size_t data_idx = idx() + sizeof(DataType);
    switch (type()) {
      case PawPrint::Data::TYPE_SINT_1B: return (T)paw_print()->getData<char>(data_idx);
      case PawPrint::Data::TYPE_UINT_1B: return (T)paw_print()->getData<byte>(data_idx);

      case PawPrint::Data::TYPE_SINT_2B: return (T)paw_print()->getData<short >(data_idx);
      case PawPrint::Data::TYPE_UINT_2B: return (T)paw_print()->getData<ushort>(data_idx);

      case PawPrint::Data::TYPE_SINT_4B: return (T)paw_print()->getData<int >(data_idx);
      case PawPrint::Data::TYPE_UINT_4B: return (T)paw_print()->getData<uint>(data_idx);

      case PawPrint::Data::TYPE_SINT_8B: return (T)paw_print()->getData<int64 >(data_idx);
      case PawPrint::Data::TYPE_UINT_8B: return (T)paw_print()->getData<uint64>(data_idx);

      case PawPrint::Data::TYPE_REAL_4B: return (T)paw_print()->getData<float>(data_idx);

      case PawPrint::Data::TYPE_REAL_8B: return (T)paw_print()->getData<double>(data_idx);

      default:
        cout << "unhandled PawPrint::Data on Cursor::_getConvertedData() --- " << type() << endl;
        return default_value;
    }
  }

  // for string
  template <class T>
  /*T*/std::enable_if_t<std::is_same_v<std::string, T>, T>
  _getConvertedData (T const& default_value) const {
    // string case
    if (isNumber() == false)
      return paw_print_->getStrValue(idx_);


    // number case
    size_t data_idx = idx() + sizeof(DataType);
    switch (type()) {
      case PawPrint::Data::TYPE_SINT_1B: return std::to_string(paw_print()->getData<char>(data_idx));
      case PawPrint::Data::TYPE_UINT_1B: return std::to_string(paw_print()->getData<byte>(data_idx));

      case PawPrint::Data::TYPE_SINT_2B: return std::to_string(paw_print()->getData<short >(data_idx));
      case PawPrint::Data::TYPE_UINT_2B: return std::to_string(paw_print()->getData<ushort>(data_idx));

      case PawPrint::Data::TYPE_SINT_4B: return std::to_string(paw_print()->getData<int >(data_idx));
      case PawPrint::Data::TYPE_UINT_4B: return std::to_string(paw_print()->getData<uint>(data_idx));

      case PawPrint::Data::TYPE_SINT_8B: return std::to_string(paw_print()->getData<int64 >(data_idx));
      case PawPrint::Data::TYPE_UINT_8B: return std::to_string(paw_print()->getData<uint64>(data_idx));

      case PawPrint::Data::TYPE_REAL_4B: return std::to_string(paw_print()->getData<float>(data_idx));

      case PawPrint::Data::TYPE_REAL_8B: return std::to_string(paw_print()->getData<double>(data_idx));

      default:
        cout << "unhandled PawPrint::Data on Cursor::get() --- " << type() << endl;
        return default_value;
    }
  }
};


template<> PAW_PRINT_API bool Cursor::is<char       > () const;
template<> PAW_PRINT_API bool Cursor::is<byte       > () const;
template<> PAW_PRINT_API bool Cursor::is<bool       > () const;
template<> PAW_PRINT_API bool Cursor::is<short      > () const;
template<> PAW_PRINT_API bool Cursor::is<ushort     > () const;
template<> PAW_PRINT_API bool Cursor::is<int        > () const;
template<> PAW_PRINT_API bool Cursor::is<uint       > () const;
template<> PAW_PRINT_API bool Cursor::is<int64      > () const;
template<> PAW_PRINT_API bool Cursor::is<uint64     > () const;
template<> PAW_PRINT_API bool Cursor::is<float      > () const;
template<> PAW_PRINT_API bool Cursor::is<double     > () const;
template<> PAW_PRINT_API bool Cursor::is<string     > () const;
template<> PAW_PRINT_API bool Cursor::is<const char*> () const;

template<> PAW_PRINT_API bool Cursor::isConvertable<char       > () const;
template<> PAW_PRINT_API bool Cursor::isConvertable<byte       > () const;
template<> PAW_PRINT_API bool Cursor::isConvertable<bool       > () const;
template<> PAW_PRINT_API bool Cursor::isConvertable<short      > () const;
template<> PAW_PRINT_API bool Cursor::isConvertable<ushort     > () const;
template<> PAW_PRINT_API bool Cursor::isConvertable<int        > () const;
template<> PAW_PRINT_API bool Cursor::isConvertable<uint       > () const;
template<> PAW_PRINT_API bool Cursor::isConvertable<int64      > () const;
template<> PAW_PRINT_API bool Cursor::isConvertable<uint64     > () const;
template<> PAW_PRINT_API bool Cursor::isConvertable<float      > () const;
template<> PAW_PRINT_API bool Cursor::isConvertable<double     > () const;
template<> PAW_PRINT_API bool Cursor::isConvertable<string     > () const;
//template<> PAW_PRINT_API bool Cursor::isConvertable<const char*> () const;  we don't use this


class PAW_PRINT_API MergerCursor : public Cursor {
public:
  using super = Cursor;

  static bool isAvailableType (DataType type);

public:
  MergerCursor (
      unsigned short merge_level = 1,
      bool need_merge_sequence = true,
      bool need_merge_map = true
  );
  MergerCursor (
      const vector<shared_ptr<Cursor>> &cursor_stack,
      unsigned short merge_level = 1,
      bool need_merge_sequence = true,
      bool need_merge_map = true
  );


  const shared_ptr<PawPrint>& paw_print () const override {
    if (cursor_stack_.size() <= 0) {
      static shared_ptr<PawPrint> null_sp;
      return null_sp;
    }
    return cursor_stack_.back()->paw_print();
  }

  int idx () const override {
    if (cursor_stack_.size() <= 0)
      return -1;
    return cursor_stack_.back()->idx();
  }
  DataType type () const override {
    if (cursor_stack_.size() <= 0)
      return PawPrint::Data::TYPE_NONE;
    return cursor_stack_.back()->type();
  }
  int size () const override;


public:
  bool pushCursor (const shared_ptr<Cursor> &cursor);
  shared_ptr<Cursor> popCursor ();

  shared_ptr<Cursor> getElem (int idx) const override;
  shared_ptr<Cursor> getElem (const char *key) const override;
  shared_ptr<Cursor> getElem (const string &key) const override;

  shared_ptr<Cursor> getKeyValuePair (int idx) const override;

  string toString (int indent=0, int indent_inc=2, bool ignore_indent=false) const override;

  const char* getKey () const override;
  shared_ptr<Cursor> getValue () const override;

  shared_ptr<Cursor> findKeyValuePair (const char *key) const override;

  const string & getName () const override;
  int getColumn () const override;
  int getLine () const override;

  bool isValid () const override;


private:
  mutable vector<shared_ptr<Cursor>> key_value_pairs_;
  mutable string name_;

  vector<shared_ptr<Cursor>> cursor_stack_;
  mutable int size_;  // -1 if need compute
  unsigned short merge_level_;
  bool need_merge_sequence_ : 1;
  bool need_merge_map_      : 1;


  void _resetMapSizeAndKeyValuePairs () const;
};



class PAW_PRINT_API MapLoader {
public:
  using LoaderFunc = function<void(const shared_ptr<Cursor> &value)>;


  MapLoader ();
  MapLoader (const vector<std::pair<string, LoaderFunc>> &cases);


  void load (const shared_ptr<Cursor> &cursor);

  inline void addCase (const char *key, LoaderFunc func) {
    cases_.push_back(std::pair<string, LoaderFunc>(key, func));
  }

  inline size_t case_size () const { return cases_.size(); }


private:
  vector<std::pair<string, LoaderFunc>> cases_;
};


}


#endif  // EXTERNAL_PAW_PRINT_SRC_CURSOR
