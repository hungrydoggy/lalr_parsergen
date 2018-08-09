#ifndef PAW_PRINT
#define PAW_PRINT

#include <unordered_map>
#include <vector>
#include <stack>
#include <string>

#include "../../src/token.h"
#include "../../src/node.h"


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

		static const DataType TYPE_NONE = 0;
		static const DataType TYPE_INT = 1;
		static const DataType TYPE_DOUBLE = 2;
		static const DataType TYPE_STRING = 3;

		static const DataType TYPE_SEQUENCE = 4;
		static const DataType TYPE_SEQUENCE_START = 4;
		static const DataType TYPE_SEQUENCE_END = 5;

		static const DataType TYPE_MAP = 6;
		static const DataType TYPE_MAP_START = 6;
		static const DataType TYPE_MAP_END = 7;

		static const DataType TYPE_KEY_VALUE_PAIR = 8;
	};

    class Cursor {
    public:
        Cursor (const PawPrint &paw_print, int idx);

		inline int idx () const { return idx_; }
        DataType type () const;

        template <class T>
        bool is () const { return false; }

        bool isSequence () const;
        bool isMap () const;

        template <class T>
        T get (T default_value) const {
            if (is<T>() == false)
                return default_value;
            return paw_print_.getData<T>(idx_ + sizeof(DataType));
        }

        const char* get (const string &default_value) const;

        Cursor operator[] (int idx) const;
        Cursor operator[] (const char *key) const;
        Cursor operator[] (const string &key) const;

        const char* getKey (int idx) const;

        int size () const;

    private:
        const PawPrint &paw_print_;
        int idx_;
    };


    inline const vector<unsigned char>& raw_data () const {
        return raw_data_;
    }

    PawPrint ();
    PawPrint (const vector<unsigned char> &raw_data);
    ~PawPrint ();

    DataType type (int idx) const;

    int dataSize (int idx) const;

    void setRawData (const vector<unsigned char> &raw_data);

    bool tokenize (const char *text, vector<Token> &tokens);
    bool addIndentTokens (const vector<Token> &tokens, vector<Token> &indented);
    bool loadText (const char *text);


    // write
    void pushInt    (int    value); 
    void pushDouble (double value); 
    void pushString (const char *value); 
    inline void pushString (const string &value) { return pushString(value.c_str()); }
    void pushKey (const char *value);
    inline void pushKey (const string &value) { return pushKey(value.c_str()); }
    void beginSequence (); 
    void endSequence (); 
    void beginMap (); 
    void endMap (); 


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


    // return next idx
    int _parse_step (const char *text, const vector<Token> &tokens, int start_idx);
};

template<> bool PawPrint::Cursor::is<int        > () const;
template<> bool PawPrint::Cursor::is<double     > () const;
template<> bool PawPrint::Cursor::is<const char*> () const;
template<> bool PawPrint::Cursor::is<string     > () const;

template<> const char* PawPrint::Cursor::get<const char*> (const char *default_value) const;

}

#endif
