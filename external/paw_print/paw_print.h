#ifndef PAW_PRINT
#define PAW_PRINT

#include <unordered_map>
#include <vector>
#include <stack>
#include <string>

#include <token.h>
#include <node.h>


namespace paw_print {


using std::stack;
using std::string;
using std::unordered_map;
using std::vector;

using namespace parse_table;

using DataType = unsigned char;

class TokenType {
public:
    enum Type {
        END_OF_FILE,
        INDENT,
        DEDENT,
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
    };
};


class PawPrint {
public:
	class Data {
	public:
		using StrSizeType = unsigned short;

		static const DataType TYPE_NONE = 0xff;

		static const DataType TYPE_NULL = 0;

		static const DataType TYPE_BOOL   = 1;
		static const DataType TYPE_INT    = 2;
		static const DataType TYPE_DOUBLE = 3;
		static const DataType TYPE_STRING = 4;

		static const DataType TYPE_SEQUENCE       = 5;
		static const DataType TYPE_SEQUENCE_START = 5;
		static const DataType TYPE_SEQUENCE_END   = 6;

		static const DataType TYPE_MAP       = 7;
		static const DataType TYPE_MAP_START = 7;
		static const DataType TYPE_MAP_END   = 8;

		static const DataType TYPE_KEY_VALUE_PAIR = 9;
	};

    class Cursor {
    public:
        const PawPrint* paw_print () const { return paw_print_; }

        Cursor (
                const PawPrint *paw_print,
                int idx,
                const shared_ptr<PawPrint> &holdable = null);
        Cursor (const Cursor &cursor);

		inline int idx () const { return idx_; }
        DataType type () const;

        template <class T>
        bool is () const { return false; }

        template <class T>
        bool isConvertable () const { return false; }

        bool isSequence () const;
        bool isMap () const;
        bool isKeyValuePair () const;

        bool isNull () const;

        inline bool isValid () const { return idx_ >= 0; }

        template <class T>
        T get (T default_value) const {
            if (isConvertable<T>() == false)
                return default_value;
            return paw_print_->getData<T>(idx_ + sizeof(DataType));
        }

        string get (const char *default_value) const;
        string get (const string &default_value) const;

        Cursor operator[] (int idx) const;
        Cursor operator[] (const char *key) const;
        Cursor operator[] (const string &key) const;

        const Cursor& operator = (const Cursor &cursor);

        const char* getKey (int idx) const;

        int size () const;

        string toString (int indent=0, int indent_inc=2, bool ignore_indent=false) const;

        const char* getKeyOfPair () const;
        Cursor getValueOfPair () const;

        int getColumn () const;
        int getLine () const;

        inline void hold (const shared_ptr<PawPrint> &sp) {
            if (sp.get() == paw_print_)
                holder_ = sp;
        }

    private:
        const PawPrint *paw_print_;
        int idx_;
        shared_ptr<PawPrint> holder_; 
    };


    inline const vector<unsigned char>& raw_data () const {
        return raw_data_;
    }

    PawPrint ();
    PawPrint (const vector<unsigned char> &raw_data);
    PawPrint (const Cursor &cursor);
    PawPrint (bool          value);
    PawPrint (int           value);
    PawPrint (double        value);
    PawPrint (const char   *value);
    PawPrint (const string &value);

    PawPrint (const vector<int   > &value);
    PawPrint (const vector<double> &value);
    PawPrint (const vector<string> &value);

    ~PawPrint ();

    const PawPrint& operator = (const Cursor &cursor);

    DataType type (int idx) const;

    int dataSize (int idx) const;

    void setRawData (const vector<unsigned char> &raw_data);

    int getColumn (int idx) const;
    int getLine (int idx) const;


    // write
    void pushNull   (int column=-1, int line=-1); 
    void pushBool   (bool   value, int column=-1, int line=-1); 
    void pushInt    (int    value, int column=-1, int line=-1); 
    void pushDouble (double value, int column=-1, int line=-1); 
    void pushString (const char *value, int column=-1, int line=-1); 
    inline void pushString (const string &value, int column=-1, int line=-1) {
        return pushString(value.c_str(), column, line);
    }
    void pushKeyValuePair (int column=-1, int line=-1);
    void pushKey (const char *value, int column=-1, int line=-1);
    inline void pushKey (const string &value, int column=-1, int line=-1) {
        return pushKey(value.c_str(), column, line);
    }
    void beginSequence (int column=-1, int line=-1); 
    void endSequence   (int column=-1, int line=-1); 
    void beginMap (int column=-1, int line=-1); 
    void endMap   (int column=-1, int line=-1); 


    // read
    Cursor root () const;

    Data::StrSizeType getStrSize (int idx) const;
    const char* getStrValue (int idx) const;

    int getKeyRawIdxOfPair   (int pair_idx) const;
    int getValueRawIdxOfPair (int pair_idx) const;

    template <class T>
    const T& getData (int idx) const {
        return *((T*)&raw_data_[idx]);
    }

    const vector<int>& getDataIdxsOfSequence (int sequence_idx) const;
    const vector<int>& getDataIdxsOfMap (int map_idx) const;

    int findRawIdxOfValue (
            const vector<int> &map_datas,
            int first,
            int last,
            const char *key) const;

private:
    vector<unsigned char> raw_data_;
    mutable unordered_map<int, vector<int>> data_idxs_of_sequence_map_;
    mutable unordered_map<int, vector<int>> data_idxs_of_map_map_;
    bool is_closed_;

    stack<int> curly_open_idx_stack_;
    stack<int> square_open_idx_stack_;
    unordered_map<int, unsigned short> column_map_;
    unordered_map<int, unsigned short> line_map_;
};

template<> bool PawPrint::Cursor::is<bool       > () const;
template<> bool PawPrint::Cursor::is<int        > () const;
template<> bool PawPrint::Cursor::is<double     > () const;
template<> bool PawPrint::Cursor::is<const char*> () const;
template<> bool PawPrint::Cursor::is<string     > () const;

template<> bool PawPrint::Cursor::isConvertable<bool       > () const;
template<> bool PawPrint::Cursor::isConvertable<int        > () const;
template<> bool PawPrint::Cursor::isConvertable<double     > () const;
template<> bool PawPrint::Cursor::isConvertable<const char*> () const;
template<> bool PawPrint::Cursor::isConvertable<string     > () const;

template<> bool   PawPrint::Cursor::get<bool  > (bool          default_value) const;
template<> double PawPrint::Cursor::get<double> (double        default_value) const;

}

#endif
