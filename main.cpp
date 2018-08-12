
#include <assert.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>

#include "./src/parse_table.h"
#include "./src/parsing_table_generator.h"


using namespace parse_table;

using std::cout;
using std::endl;
using std::ifstream;
using std::stringstream;
using std::to_string;


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

static void _t_generateParseTree () {

    Token::to_string_func = [](const char *text, const Token *t) {
        stringstream ss;
        ss << "Token(";
        switch (t->type) {
            case TokenType::INDENT      : ss << "INDENT)"       ; return ss.str();
            case TokenType::DEDENT      : ss << "DEDENT)"       ; return ss.str();
            case TokenType::END_OF_FILE : ss << "END_OF_FILE)"  ; return ss.str();
            case TokenType::INT         : ss << "INT, "         ; break;
            case TokenType::DOUBLE      : ss << "DOUBLE, "      ; break;
            case TokenType::STRING      : ss << "STRING, "      ; break;
            case TokenType::COLON       : ss << "COLON, "       ; break;
            case TokenType::COMMA       : ss << "COMMA, "       ; break;
            case TokenType::DASH        : ss << "DASH, "        ; break;
            case TokenType::SHARP       : ss << "SHARP, "       ; break;
            case TokenType::SQUARE_OPEN : ss << "SQUARE_OPEN, " ; break;
            case TokenType::SQUARE_CLOSE: ss << "SQUARE_CLOSE, "; break;
            case TokenType::CURLY_OPEN  : ss << "CURLY_OPEN, "  ; break;
            case TokenType::CURLY_CLOSE : ss << "CURLY_CLOSE, " ; break;
        }

        auto size = t->last_idx - t->first_idx + 2;
        auto str = new char [size];
        memcpy(str, &text[t->first_idx], size-1);
        str[size-1] = 0;
        ss << t->first_idx << ", " << t->last_idx << ", indent:" << t->indent
                << ", \"" << str << "\")";
        delete[] str;
        return ss.str();
    };

	auto term_indent = make_shared<Terminal>("#indent", TokenType::INDENT);
	auto term_dedent = make_shared<Terminal>("#dedent", TokenType::DEDENT);

	auto term_int = make_shared<Terminal>("int", TokenType::INT);
	auto term_double = make_shared<Terminal>("double", TokenType::DOUBLE);
	auto term_string = make_shared<Terminal>("string", TokenType::STRING);
	auto term_colon = make_shared<Terminal>("colon", TokenType::COLON);
	auto term_comma = make_shared<Terminal>("comma", TokenType::COMMA);
	auto term_curly_open = make_shared<Terminal>("curly_open", TokenType::CURLY_OPEN);
	auto term_curly_close = make_shared<Terminal>("curly_close", TokenType::CURLY_CLOSE);

	auto non_kv = make_shared<Nonterminal>("KV");
	auto non_map = make_shared<Nonterminal>("MAP");

	auto non_kv_blocked  = make_shared<Nonterminal>("KV_BLOCKED" );
	auto non_map_blocked = make_shared<Nonterminal>("MAP_BLOCKED");

	auto non_sequence = make_shared<Nonterminal>("SEQUENCE");

	auto non_node = make_shared<Nonterminal>("NODE");

	auto start = make_shared<Nonterminal>("S");

	ParsingTableGenerator generator;
	generator.addSymbol(start, true);
	generator.addSymbol(non_kv);
	generator.addSymbol(non_map);
    generator.addSymbol(non_kv_blocked);
	generator.addSymbol(non_map_blocked);
	generator.addSymbol(non_sequence);
	generator.addSymbol(non_node);

	// KV
	non_kv->rules.push_back(
		Rule(non_kv, {
			term_string,
			term_colon ,
			term_indent,
			non_node   ,
			term_dedent,
			}));

	// MAP
	non_map->rules.push_back(
            Rule(non_map, { term_curly_open, term_curly_close }));
	non_map->rules.push_back(
            Rule(non_map, { term_curly_open, non_map_blocked, term_curly_close }));
	non_map->rules.push_back(Rule(non_map, { non_kv , non_map }));
	non_map->rules.push_back(Rule(non_map, { non_kv           }));

    // KV_BLOCKED
    non_kv_blocked->rules.push_back(
            Rule(non_kv_blocked, { term_string, term_colon, non_node }));

	// MAP_BLOCKED
    non_map_blocked->rules.push_back(
            Rule(non_map_blocked, { non_kv_blocked }));
    non_map_blocked->rules.push_back(
            Rule(non_map_blocked, { non_kv_blocked, term_comma, non_map_blocked }));

	// SEQUENCE


	// NODE
	non_node->rules.push_back(Rule(non_node, { term_int    }));
	non_node->rules.push_back(Rule(non_node, { term_double }));
	non_node->rules.push_back(Rule(non_node, { term_string }));
	non_node->rules.push_back(Rule(non_node, { non_map     }));


	start->rules.push_back(Rule(start, { non_node }));


	auto parsing_table = generator.generateTable();
    auto table_str = parsing_table->toString();
    //cout << table_str;

    auto table_correct =
		"##### Rules\n" \
		"# Rule 0 : S' -> S \n" \
		"# Rule 1 : S -> NODE \n" \
		"# Rule 2 : KV -> string colon #indent NODE #dedent \n" \
		"# Rule 3 : MAP -> curly_open curly_close \n" \
		"# Rule 4 : MAP -> curly_open MAP_BLOCKED curly_close \n" \
		"# Rule 5 : MAP -> KV MAP \n" \
		"# Rule 6 : MAP -> KV \n" \
		"# Rule 7 : KV_BLOCKED -> string colon NODE \n" \
		"# Rule 8 : MAP_BLOCKED -> KV_BLOCKED \n" \
		"# Rule 9 : MAP_BLOCKED -> KV_BLOCKED comma MAP_BLOCKED \n" \
		"# Rule 10 : NODE -> int \n" \
		"# Rule 11 : NODE -> double \n" \
		"# Rule 12 : NODE -> string \n" \
		"# Rule 13 : NODE -> MAP \n" \
		"##### Table\n" \
		"    |   $ | #dedent | #indent | colon | comma | curly_close | curly_open | double | int | string |  KV | KV_BLOCKED |  MAP | MAP_BLOCKED | NODE |   S | \n" \
		"--------------------------------------------------------------------------------------------------------------------------------------------------------\n" \
		"  0 |     |         |         |       |       |             |         s4 |     s2 |  s1 |     s3 | go5 |            |  go6 |             |  go7 | go8 | \n" \
		"  1 | r10 |     r10 |         |       |   r10 |         r10 |            |        |     |        |     |            |      |             |      |     | \n" \
		"  2 | r11 |     r11 |         |       |   r11 |         r11 |            |        |     |        |     |            |      |             |      |     | \n" \
		"  3 | r12 |     r12 |         |    s9 |   r12 |         r12 |            |        |     |        |     |            |      |             |      |     | \n" \
		"  4 |     |         |         |       |       |         s11 |            |        |     |    s10 |     |       go12 |      |        go13 |      |     | \n" \
		"  5 |  r6 |      r6 |         |       |    r6 |          r6 |         s4 |        |     |    s14 | go5 |            | go15 |             |      |     | \n" \
		"  6 | r13 |     r13 |         |       |   r13 |         r13 |            |        |     |        |     |            |      |             |      |     | \n" \
		"  7 |  r1 |         |         |       |       |             |            |        |     |        |     |            |      |             |      |     | \n" \
		"  8 | acc |         |         |       |       |             |            |        |     |        |     |            |      |             |      |     | \n" \
		"  9 |     |         |     s16 |       |       |             |            |        |     |        |     |            |      |             |      |     | \n" \
		" 10 |     |         |         |   s17 |       |             |            |        |     |        |     |            |      |             |      |     | \n" \
		" 11 |  r3 |      r3 |         |       |    r3 |          r3 |            |        |     |        |     |            |      |             |      |     | \n" \
		" 12 |     |         |         |       |   s18 |          r8 |            |        |     |        |     |            |      |             |      |     | \n" \
		" 13 |     |         |         |       |       |         s19 |            |        |     |        |     |            |      |             |      |     | \n" \
		" 14 |     |         |         |    s9 |       |             |            |        |     |        |     |            |      |             |      |     | \n" \
		" 15 |  r5 |      r5 |         |       |    r5 |          r5 |            |        |     |        |     |            |      |             |      |     | \n" \
		" 16 |     |         |         |       |       |             |         s4 |     s2 |  s1 |     s3 | go5 |            |  go6 |             | go20 |     | \n" \
		" 17 |     |         |         |       |       |             |         s4 |     s2 |  s1 |     s3 | go5 |            |  go6 |             | go21 |     | \n" \
		" 18 |     |         |         |       |       |             |            |        |     |    s10 |     |       go12 |      |        go22 |      |     | \n" \
		" 19 |  r4 |      r4 |         |       |    r4 |          r4 |            |        |     |        |     |            |      |             |      |     | \n" \
		" 20 |     |     s23 |         |       |       |             |            |        |     |        |     |            |      |             |      |     | \n" \
		" 21 |     |         |         |       |    r7 |          r7 |            |        |     |        |     |            |      |             |      |     | \n" \
		" 22 |     |         |         |       |       |          r9 |            |        |     |        |     |            |      |             |      |     | \n" \
		" 23 |  r2 |      r2 |         |       |    r2 |          r2 |         r2 |        |     |     r2 |     |            |      |             |      |     | \n";
    assert(table_str == table_correct);

    // tokens
    vector<Token> tokens = {
        Token(5, 0, 0, 0),
        Token(6, 1, 1, 0),
        Token(1, 0, 0, 0),
        Token(5, 7, 7, 4),
        Token(6, 8, 8, 4),
        Token(1, 0, 0, 0),
        Token(5, 11, 13, 8),
        Token(2, 0, 0, 0),
        Token(5, 20, 20, 4),
        Token(6, 21, 21, 4),
        Token(1, 0, 0, 0),
        Token(5, 31, 31, 8),
        Token(6, 32, 32, 8),
        Token(1, 0, 0, 0),
        Token(4, 34, 36, 12),
        Token(2, 0, 0, 0),
        Token(5, 46, 46, 8),
        Token(6, 47, 47, 8),
        Token(1, 0, 0, 0),
        Token(4, 49, 51, 12),
        Token(2, 0, 0, 0),
        Token(5, 61, 61, 8),
        Token(6, 62, 62, 8),
        Token(1, 0, 0, 0),
        Token(12, 64, 64, 12),
        Token(5, 66, 66, 12),
        Token(6, 67, 67, 12),
        Token(5, 69, 69, 16),
        Token(7, 71, 71, 16),
        Token(5, 73, 73, 16),
        Token(6, 74, 74, 16),
        Token(5, 76, 76, 20),
        Token(7, 78, 78, 20),
        Token(5, 80, 80, 20),
        Token(6, 81, 81, 20),
        Token(5, 84, 84, 24),
        Token(13, 86, 86, 24),
        Token(2, 0, 0, 0),
        Token(2, 0, 0, 0),
        Token(5, 92, 92, 4),
        Token(6, 93, 93, 4),
        Token(1, 0, 0, 0),
        Token(3, 95, 96, 8),
        Token(2, 0, 0, 0),
        Token(2, 0, 0, 0),
        Token(0, 0, 0, 0),
    };
    
    // make parse tree
	ifstream is("../example/map_05.paw", std::ifstream::binary);

	// get length of file:
	is.seekg(0, is.end);
	int size = is.tellg();
	is.seekg(0, is.beg);
	// allocate memory:
	char* content = new char[size + 2];
	char* text    = new char[size + 2];
	// read data as a block:
	is.read(content, size);
	content[size] = '\n';
	content[size + 1] = 0;
	is.close();

	int ci = 0, ti = 0;
	while (true) {
		auto c = content[ci];
		if (c == '\r') {
			++ci;
			continue;
		}

		text[ti] = c;
		if (c == 0)
			break;

		++ci;
		++ti;
	}

    auto root_node = parsing_table->generateParseTree(text, tokens);
    assert(root_node != null);

	auto node_str = root_node->toString(text, 0, true);
    //cout << node_str << endl;

	auto node_correct =
		"Nonterminal(\"S\")\n" \
		"|-Nonterminal(\"NODE\")\n" \
		"|-|-Nonterminal(\"MAP\")\n" \
		"|-|-|-Nonterminal(\"KV\")\n" \
		"|-|-|-|-Terminal(\"string\", Token(STRING, 0, 0, indent:0, \"a\"))\n" \
		"|-|-|-|-Terminal(\"colon\", Token(COLON, 1, 1, indent:0, \":\"))\n" \
		"|-|-|-|-Terminal(\"#indent\", Token(INDENT))\n" \
		"|-|-|-|-Nonterminal(\"NODE\")\n" \
		"|-|-|-|-|-Nonterminal(\"MAP\")\n" \
		"|-|-|-|-|-|-Nonterminal(\"KV\")\n" \
		"|-|-|-|-|-|-|-Terminal(\"string\", Token(STRING, 7, 7, indent:4, \"b\"))\n" \
		"|-|-|-|-|-|-|-Terminal(\"colon\", Token(COLON, 8, 8, indent:4, \":\"))\n" \
		"|-|-|-|-|-|-|-Terminal(\"#indent\", Token(INDENT))\n" \
		"|-|-|-|-|-|-|-Nonterminal(\"NODE\")\n" \
		"|-|-|-|-|-|-|-|-Terminal(\"string\", Token(STRING, 11, 13, indent:8, \"abc\"))\n" \
		"|-|-|-|-|-|-|-Terminal(\"#dedent\", Token(DEDENT))\n" \
		"|-|-|-|-|-|-Nonterminal(\"MAP\")\n" \
		"|-|-|-|-|-|-|-Nonterminal(\"KV\")\n" \
		"|-|-|-|-|-|-|-|-Terminal(\"string\", Token(STRING, 20, 20, indent:4, \"c\"))\n" \
		"|-|-|-|-|-|-|-|-Terminal(\"colon\", Token(COLON, 21, 21, indent:4, \":\"))\n" \
		"|-|-|-|-|-|-|-|-Terminal(\"#indent\", Token(INDENT))\n" \
		"|-|-|-|-|-|-|-|-Nonterminal(\"NODE\")\n" \
		"|-|-|-|-|-|-|-|-|-Nonterminal(\"MAP\")\n" \
		"|-|-|-|-|-|-|-|-|-|-Nonterminal(\"KV\")\n" \
		"|-|-|-|-|-|-|-|-|-|-|-Terminal(\"string\", Token(STRING, 31, 31, indent:8, \"x\"))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-Terminal(\"colon\", Token(COLON, 32, 32, indent:8, \":\"))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-Terminal(\"#indent\", Token(INDENT))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-Nonterminal(\"NODE\")\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"double\", Token(DOUBLE, 34, 36, indent:12, \"1.0\"))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-Terminal(\"#dedent\", Token(DEDENT))\n" \
		"|-|-|-|-|-|-|-|-|-|-Nonterminal(\"MAP\")\n" \
		"|-|-|-|-|-|-|-|-|-|-|-Nonterminal(\"KV\")\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"string\", Token(STRING, 46, 46, indent:8, \"y\"))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"colon\", Token(COLON, 47, 47, indent:8, \":\"))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"#indent\", Token(INDENT))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-Nonterminal(\"NODE\")\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"double\", Token(DOUBLE, 49, 51, indent:12, \"2.0\"))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"#dedent\", Token(DEDENT))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-Nonterminal(\"MAP\")\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-Nonterminal(\"KV\")\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"string\", Token(STRING, 61, 61, indent:8, \"z\"))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"colon\", Token(COLON, 62, 62, indent:8, \":\"))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"#indent\", Token(INDENT))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-Nonterminal(\"NODE\")\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-Nonterminal(\"MAP\")\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"curly_open\", Token(CURLY_OPEN, 64, 64, indent:12, \"{\"))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Nonterminal(\"MAP_BLOCKED\")\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Nonterminal(\"KV_BLOCKED\")\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"string\", Token(STRING, 66, 66, indent:12, \"i\"))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"colon\", Token(COLON, 67, 67, indent:12, \":\"))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Nonterminal(\"NODE\")\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"string\", Token(STRING, 69, 69, indent:16, \"1\"))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"comma\", Token(COMMA, 71, 71, indent:16, \",\"))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Nonterminal(\"MAP_BLOCKED\")\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Nonterminal(\"KV_BLOCKED\")\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"string\", Token(STRING, 73, 73, indent:16, \"j\"))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"colon\", Token(COLON, 74, 74, indent:16, \":\"))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Nonterminal(\"NODE\")\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"string\", Token(STRING, 76, 76, indent:20, \"2\"))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"comma\", Token(COMMA, 78, 78, indent:20, \",\"))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Nonterminal(\"MAP_BLOCKED\")\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Nonterminal(\"KV_BLOCKED\")\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"string\", Token(STRING, 80, 80, indent:20, \"k\"))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"colon\", Token(COLON, 81, 81, indent:20, \":\"))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Nonterminal(\"NODE\")\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"string\", Token(STRING, 84, 84, indent:24, \"3\"))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"curly_close\", Token(CURLY_CLOSE, 86, 86, indent:24, \"}\"))\n" \
		"|-|-|-|-|-|-|-|-|-|-|-|-|-Terminal(\"#dedent\", Token(DEDENT))\n" \
		"|-|-|-|-|-|-|-|-Terminal(\"#dedent\", Token(DEDENT))\n" \
		"|-|-|-|-|-|-|-Nonterminal(\"MAP\")\n" \
		"|-|-|-|-|-|-|-|-Nonterminal(\"KV\")\n" \
		"|-|-|-|-|-|-|-|-|-Terminal(\"string\", Token(STRING, 92, 92, indent:4, \"d\"))\n" \
		"|-|-|-|-|-|-|-|-|-Terminal(\"colon\", Token(COLON, 93, 93, indent:4, \":\"))\n" \
		"|-|-|-|-|-|-|-|-|-Terminal(\"#indent\", Token(INDENT))\n" \
		"|-|-|-|-|-|-|-|-|-Nonterminal(\"NODE\")\n" \
		"|-|-|-|-|-|-|-|-|-|-Terminal(\"int\", Token(INT, 95, 96, indent:8, \"13\"))\n" \
		"|-|-|-|-|-|-|-|-|-Terminal(\"#dedent\", Token(DEDENT))\n" \
		"|-|-|-|-Terminal(\"#dedent\", Token(DEDENT))";
	assert(node_str == node_correct);

    vector<unsigned char> result;
    parsing_table->saveBinary(result);

	auto loaded = ParsingTable(result);

	auto rn = loaded.generateParseTree(text, tokens);
	assert(rn != null);

	auto rn_str = rn->toString(text, 0, true);
	//cout << rn_str << endl;
	assert(node_str == rn_str);

	delete[] content;
	delete[] text;
}

static void _t_generatePawPrintParsingTable () {
    Token::to_string_func = [](const char *text, const Token *t) {
        stringstream ss;
        ss << "Token(";
        switch (t->type) {
            case TokenType::INDENT      : ss << "INDENT)"       ; return ss.str();
            case TokenType::DEDENT      : ss << "DEDENT)"       ; return ss.str();
            case TokenType::END_OF_FILE : ss << "END_OF_FILE)"  ; return ss.str();
            case TokenType::INT         : ss << "INT, "         ; break;
            case TokenType::DOUBLE      : ss << "DOUBLE, "      ; break;
            case TokenType::STRING      : ss << "STRING, "      ; break;
            case TokenType::COLON       : ss << "COLON, "       ; break;
            case TokenType::COMMA       : ss << "COMMA, "       ; break;
            case TokenType::DASH        : ss << "DASH, "        ; break;
            case TokenType::SHARP       : ss << "SHARP, "       ; break;
            case TokenType::SQUARE_OPEN : ss << "SQUARE_OPEN, " ; break;
            case TokenType::SQUARE_CLOSE: ss << "SQUARE_CLOSE, "; break;
            case TokenType::CURLY_OPEN  : ss << "CURLY_OPEN, "  ; break;
            case TokenType::CURLY_CLOSE : ss << "CURLY_CLOSE, " ; break;
        }

        auto size = t->last_idx - t->first_idx + 2;
        auto str = new char [size];
        memcpy(str, &text[t->first_idx], size-1);
        str[size-1] = 0;
        ss << t->first_idx << ", " << t->last_idx << ", indent:" << t->indent
                << ", \"" << str << "\")";
        delete[] str;
        return ss.str();
    };

	auto term_indent = make_shared<Terminal>("#indent", TokenType::INDENT);
	auto term_dedent = make_shared<Terminal>("#dedent", TokenType::DEDENT);

	auto term_int    = make_shared<Terminal>("int"   , TokenType::INT   );
	auto term_double = make_shared<Terminal>("double", TokenType::DOUBLE);
	auto term_string = make_shared<Terminal>("string", TokenType::STRING);
	auto term_colon  = make_shared<Terminal>("colon" , TokenType::COLON );
	auto term_comma  = make_shared<Terminal>("comma" , TokenType::COMMA );
	auto term_dash   = make_shared<Terminal>("dash"  , TokenType::DASH  );
	auto term_curly_open  = make_shared<Terminal>("curly_open" , TokenType::CURLY_OPEN );
	auto term_curly_close = make_shared<Terminal>("curly_close", TokenType::CURLY_CLOSE);
	auto term_square_open  = make_shared<Terminal>("square_open" , TokenType::SQUARE_OPEN );
	auto term_square_close = make_shared<Terminal>("square_close", TokenType::SQUARE_CLOSE);

	auto non_kv  = make_shared<Nonterminal>("KV" );
	auto non_map = make_shared<Nonterminal>("MAP");

	auto non_kv_blocked  = make_shared<Nonterminal>("KV_BLOCKED" );
	auto non_map_blocked = make_shared<Nonterminal>("MAP_BLOCKED");

    auto non_seq_elem = make_shared<Nonterminal>("SEQ_ELEM");
	auto non_sequence = make_shared<Nonterminal>("SEQUENCE");

	auto non_node = make_shared<Nonterminal>("NODE");

	auto start = make_shared<Nonterminal>("S");

	ParsingTableGenerator generator;
	generator.addSymbol(start, true);
	generator.addSymbol(non_kv);
	generator.addSymbol(non_map);
    generator.addSymbol(non_kv_blocked);
	generator.addSymbol(non_map_blocked);
	generator.addSymbol(non_seq_elem);
	generator.addSymbol(non_sequence);
	generator.addSymbol(non_node);

	// KV
	non_kv->rules.push_back(
		Rule(non_kv, {
			term_string,
			term_colon ,
			term_indent,
			non_node   ,
			term_dedent,
			}));

	// MAP
	non_map->rules.push_back(
            Rule(non_map, { term_curly_open, term_curly_close }));
	non_map->rules.push_back(
            Rule(non_map, { term_curly_open, non_map_blocked, term_curly_close }));
	non_map->rules.push_back(Rule(non_map, { non_kv , non_map }));
	non_map->rules.push_back(Rule(non_map, { non_kv           }));

    // KV_BLOCKED
    non_kv_blocked->rules.push_back(
            Rule(non_kv_blocked, { term_string, term_colon, non_node }));

	// MAP_BLOCKED
    non_map_blocked->rules.push_back(
            Rule(non_map_blocked, { non_kv_blocked }));
    non_map_blocked->rules.push_back(
            Rule(non_map_blocked, { non_kv_blocked, term_comma, non_map_blocked }));

    // SEQ_ELEM
    non_seq_elem->rules.push_back(
            Rule(non_seq_elem, { term_dash, non_node }));

	// SEQUENCE
    non_sequence->rules.push_back(
            Rule(non_sequence, { non_seq_elem, non_sequence }));
    non_sequence->rules.push_back(
            Rule(non_sequence, { non_seq_elem               }));


	// NODE
	non_node->rules.push_back(Rule(non_node, { term_int     }));
	non_node->rules.push_back(Rule(non_node, { term_double  }));
	non_node->rules.push_back(Rule(non_node, { term_string  }));
	non_node->rules.push_back(Rule(non_node, { non_map      }));
	non_node->rules.push_back(Rule(non_node, { non_sequence }));


	start->rules.push_back(Rule(start, { non_node }));


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
	_t_generateParseTree();
	_t_generatePawPrintParsingTable();
    return 0;
}
