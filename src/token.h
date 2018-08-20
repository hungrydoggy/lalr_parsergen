#ifndef PAW_PRINT_TOKEN
#define PAW_PRINT_TOKEN

#include <functional>
#include <string>

#include "./defines.h"

namespace parse_table {

using std::function;
using std::string;

class PAW_PRINT_API Token {
public:
    static function<string(const char *text, const Token *token)> to_string_func;

    int type;
    int first_idx;
    int last_idx;
    unsigned short indent;
    unsigned short column;
    unsigned short line;


    Token (
            int type,
            int first_idx,
            int last_idx,
            unsigned short indent,
            unsigned short column,
            unsigned short line);

    string toString (const char *text) const;
};

}

#include "./undefines.h"

#endif
