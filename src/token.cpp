#include "token.h"

#include <sstream>


namespace parse_table {

using std::stringstream;

function<string(const char *text, const Token *token)> Token::to_string_func;

Token::Token (
        int type,
        int first_idx,
        int last_idx,
        unsigned short indent,
        unsigned short column,
        unsigned short line)
:type(type),
 first_idx(first_idx),
 last_idx(last_idx),
 indent(indent),
 column(column),
 line(line) {
}

string Token::toString (const char *text) const {
    if (to_string_func == null)
        return "";

    return to_string_func(text, this);
}


}
