
#include <assert.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>

#include "../src/parse_table.h"
#include "../src/parsing_table_generator.h"


using namespace parse_table;

using std::cout;
using std::endl;
using std::ifstream;
using std::stringstream;
using std::to_string;



class PAW_PRINT_API TokenType {
public:
    enum Type {
        END_OF_FILE,
        INDENT,
        DEDENT,
        BOOL,
        INT,
        FLOAT,
        DOUBLE,
        STRING,
        COLON,
        COMMA,
        DOT,
        PLUS,
        MINUS,
        MULTIPLY,
        DIVIDE,
        SHARP,
        SQUARE_OPEN,
        SQUARE_CLOSE,
        CURLY_OPEN,
        CURLY_CLOSE,
        NEW_LINE,
    };
};


static void _t_generateBunnyLangParsingTable () {
    Token::to_string_func = [](const char *text, const Token *t) {
        stringstream ss;
        ss << "Token(";
        switch (t->type) {
            case TokenType::INDENT: ss << "INDENT)"; return ss.str();
            case TokenType::DEDENT: ss << "DEDENT)"; return ss.str();
            case TokenType::END_OF_FILE: ss << "END_OF_FILE)"; return ss.str();
#define ST(TYPE) case TokenType::TYPE: ss << #TYPE", "; break;
            ST(BOOL) ST(INT) ST(FLOAT) ST(DOUBLE) ST(STRING)
            ST(COLON) ST(COMMA) ST(DOT)
            ST(PLUS) ST(MINUS) ST(MULTIPLY) ST(DIVIDE)
            ST(SHARP)
            ST(SQUARE_OPEN) ST(SQUARE_CLOSE)
            ST(CURLY_OPEN) ST(CURLY_CLOSE)
            ST(NEW_LINE)
#undef ST
        }

        auto size = t->last_idx - t->first_idx + 2;
        auto str = new char[size];
        memcpy(str, &text[t->first_idx], size-1);
        str[size-1] = 0;
        ss << t->first_idx << ", " << t->last_idx << ", indent:" << t->indent
            << ", \"" << str << "\")";
        delete[] str;
        return ss.str();
    };

    // term
    auto t_indent   = make_shared<Terminal>("#indent", TokenType::INDENT);
    auto t_dedent   = make_shared<Terminal>("#dedent", TokenType::DEDENT);
    auto t_new_line = make_shared<Terminal>("#new_line", TokenType::NEW_LINE);

    auto t_bool         = make_shared<Terminal>("bool"        , TokenType::BOOL        );
    auto t_int          = make_shared<Terminal>("int"         , TokenType::INT         );
    auto t_float        = make_shared<Terminal>("float"       , TokenType::FLOAT       );
    auto t_double       = make_shared<Terminal>("double"      , TokenType::DOUBLE      );
    auto t_string       = make_shared<Terminal>("string"      , TokenType::STRING      );
    auto t_colon        = make_shared<Terminal>("colon"       , TokenType::COLON       );
    auto t_comma        = make_shared<Terminal>("comma"       , TokenType::COMMA       );
    auto t_dot          = make_shared<Terminal>("dot"         , TokenType::DOT         );
    auto t_plus         = make_shared<Terminal>("plus"        , TokenType::PLUS        );
    auto t_minus        = make_shared<Terminal>("minus"       , TokenType::MINUS       );
    auto t_multiply     = make_shared<Terminal>("multiply"    , TokenType::MULTIPLY    );
    auto t_divide       = make_shared<Terminal>("divide"      , TokenType::DIVIDE      );
    auto t_curly_open   = make_shared<Terminal>("curly_open"  , TokenType::CURLY_OPEN  );
    auto t_curly_close  = make_shared<Terminal>("curly_close" , TokenType::CURLY_CLOSE );
    auto t_square_open  = make_shared<Terminal>("square_open" , TokenType::SQUARE_OPEN );
    auto t_square_close = make_shared<Terminal>("square_close", TokenType::SQUARE_CLOSE);


    ParsingTableGenerator generator;


    // non term
    auto n_S = make_shared<Nonterminal>("S");
    generator.addSymbol(n_S, true);
#define ADD_NON(NAME) auto n_##NAME = make_shared<Nonterminal>(#NAME); generator.addSymbol(n_##NAME)
    ADD_NON(EXPRESSION);
#undef ADD_NON

    // EXPRESSION
#define MAKE_NON_NAMES_0() 
#define MAKE_NON_NAMES_1(NAME) n_##NAME
#define MAKE_NON_NAMES_2(NAME, ...) MAKE_NON_NAMES_1(NAME), MAKE_NON_NAMES_1(__VA_ARGS__)
#define MAKE_NON_NAMES_3(NAME, ...) MAKE_NON_NAMES_1(NAME), MAKE_NON_NAMES_2(__VA_ARGS__)
#define MAKE_NON_NAMES_(N, ...) CONCATENATE(MAKE_NON_NAMES_, N)(__VA_ARGS__)
#define MAKE_NON_NAMES(...) MAKE_NON_NAMES_(ARG_CNT(__VA_ARGS__), __VA_ARGS__)
#define ADD_RULE(LEFT, ...) n_##LEFT->rules.push_back(Rule(n_##LEFT, {MAKE_NON_NAMES(__VA_ARGS__)}))
    //ADD_RULE(EXPRESSION,    EXPRESSION, OPERATOR, EXPRESSION);
#undef ADD_RULE

#define COUNT_ARGUMENTS(...) ELEVENTH_ARGUMENT_(dummy, __VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define ELEVENTH_ARGUMENT(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, ...) a11
#define ELEVENTH_ARGUMENT_(...) ELEVENTH_ARGUMENT(__VA_ARGS__)
#define EXPAND( x ) x

#define VA_NARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, N, ...) N
#define VA_NARGS(...) VA_NARGS_IMPL(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)


#define VAR_MACRO(...) __VA_ARGS__
#define _GET_NTH_ARG(_1, _2, _3, _4, _5, N, ...) N
#define COUNT_VARARGS(...) _GET_NTH_ARG("ignored", __VA_ARGS__, 4, 3, 2, 1, 0)

    auto a = COUNT_VARARGS("one", "two", 3, 4, 5);

    auto parsing_table = generator.generateTable();
    auto table_str = parsing_table->toString();
    cout << table_str;

    vector<unsigned char> result;
    parsing_table->saveBinary(result);

    auto loaded = ParsingTable(result);
    auto loaded_str = loaded.toString();
    //cout << loaded_str;
    assert(loaded_str == table_str);

    // save
    std::ofstream f;
    f.open("paw_print.tab", std::ofstream::out | std::ofstream::binary);
    f.write((char*)result.data(), result.size());
    f.close();
}

int main () {
    _t_generateBunnyLangParsingTable();
    return 0;
}
