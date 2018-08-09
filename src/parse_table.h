#ifndef PAW_PRINT_PARSE_TABLE
#define PAW_PRINT_PARSE_TABLE

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "token.h"
#include "node.h"

#include "defines.h"

namespace parse_table {

using std::map;
using std::set;
using std::shared_ptr;
using std::vector;

using FirstMap = map<shared_ptr<TerminalBase>, set<shared_ptr<TerminalBase>>>;


class Configuration {
public:
	PAW_GETTER(const shared_ptr<Nonterminal>&, left_side)
	PAW_GETTER(const Rule&, rule)
	PAW_GETTER(int, idx_after_cursor)

	inline set<shared_ptr<TerminalBase>>& lookahead () { return lookahead_; }


	Configuration (
			const shared_ptr<Nonterminal> &left_side,
			const Rule &rule,
			int idx_after_cursor,
			const set<shared_ptr<TerminalBase>> &lookahead);

	string toString ();


private:
	shared_ptr<Nonterminal> left_side_;
	const Rule &rule_;
	int idx_after_cursor_;
	set<shared_ptr<TerminalBase>> lookahead_; // null means end($)
};

class State {
public:
	static shared_ptr<State> makeState(
			const vector<shared_ptr<Nonterminal>> &all_terminals,
			const FirstMap &first_map,
			const vector<shared_ptr<Configuration>> &transited_configs);

	PAW_GETTER_SETTER(const string &, name)
	PAW_GETTER(const vector<shared_ptr<Configuration>>&, transited_configs)
	PAW_GETTER(const vector<shared_ptr<Configuration>>&, closures)
	inline map<shared_ptr<TerminalBase>, shared_ptr<State>>& transition_map () {
        return transition_map_;
    }

	State (
			const vector<shared_ptr<Configuration>> &transited_configs,
			const vector<shared_ptr<Configuration>> &closures);

	void print ();


private:
	string name_;
	vector<shared_ptr<Configuration>> transited_configs_;
	vector<shared_ptr<Configuration>> closures_;
	map<shared_ptr<TerminalBase>, shared_ptr<State>> transition_map_;
};

class ParsingTable {
public:
	class ActionInfo {
	public:
		enum Action {
			NONE,
			SHIFT,
			REDUCE,
			GOTO,
			ACCEPT,
		};

		Action action;
		int idx;

		ActionInfo ();
		ActionInfo (Action action, int idx);
	};

	ParsingTable (const vector<unsigned char> &data);

	ParsingTable (
			const vector<shared_ptr<Nonterminal>> &symbols,
			const shared_ptr<Nonterminal> &start_symbol,
			const vector<shared_ptr<State>> &states);

    string toString () const;

    shared_ptr<Node> generateParseTree (
            const char *text,
            const vector<Token> &tokens,
            bool need_print=false);

    bool saveBinary (vector<unsigned char> &result);

private:
    vector<shared_ptr<Nonterminal>> symbols_;
    shared_ptr<Nonterminal> start_symbol_;
    map<int, shared_ptr<Terminal>> terminal_map_; // token_type -> terminal
	vector<const Rule*> rules_;
	vector<map<shared_ptr<TerminalBase>, ActionInfo>> action_info_map_list_;
};

#include "undefines.h"
}

#endif
