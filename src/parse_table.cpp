#include "parse_table.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <paw_print.h>


namespace parse_table {


using std::cout;
using std::dynamic_pointer_cast;
using std::endl;
using std::make_shared;
using std::setfill;
using std::setw;
using std::sort;
using std::stringstream;
using std::to_string;

using namespace paw_print;


Configuration::Configuration (
		const shared_ptr<Nonterminal> &left_side,
		const Rule &rule,
		int idx_after_cursor,
		const set<shared_ptr<TerminalBase>> &lookahead)
:left_side_(left_side),
 rule_(rule),
 idx_after_cursor_(idx_after_cursor),
 lookahead_(lookahead) {

}

string Configuration::toString () {
    stringstream ss;
	ss << left_side_->name << " -> ";
	for (int ri = 0; ri < rule_.right_side.size(); ++ri) {
		auto &termnon = rule_.right_side[ri];
		if (ri == idx_after_cursor_)
			ss << ".";
		ss << termnon->name;
		if (ri < rule_.right_side.size() - 1)
			ss << " ";
	}

	if (idx_after_cursor_ >= rule_.right_side.size())
		ss << ".";

	ss << "  , ";
	int li = 0;
	for (auto &termnon : lookahead_) {
		if (li != 0)
			ss << " / ";
		if (termnon == null)
			ss << "$";
		else
			ss << termnon->name;
		++li;
	}

	ss << endl;

    return ss.str();
}

State::State (
		const vector<shared_ptr<Configuration>> &transited_configs,
		const vector<shared_ptr<Configuration>> &closures)
:transited_configs_(transited_configs),
 closures_(closures) {

}

static void _addClosures (
		const FirstMap &first_map,
		const shared_ptr<Configuration> &c,
		set<shared_ptr<Nonterminal>> &non_set,
		vector<shared_ptr<Configuration>> &closures) {
	auto &rule = c->rule();
	auto idx_after_cursor = c->idx_after_cursor();
	if (idx_after_cursor >= rule.right_side.size())
		return;

	// find non after cursor
	auto &termnon = rule.right_side[idx_after_cursor];
	if (termnon->isTerminal() == true)
		return;
	auto non = dynamic_pointer_cast<Nonterminal>(termnon);
	

	// make lookahead
	set<shared_ptr<TerminalBase>> lookahead;
	if (idx_after_cursor + 1 >= rule.right_side.size()) {
		// if next isn't exist, use lookahead
		lookahead = c->lookahead();
	}else {
		// use first(next)
		auto &next = rule.right_side[idx_after_cursor + 1];
		if (next->isTerminal() == true)
			lookahead.insert(next);
		else
			lookahead = first_map.at(next);
	}


	// add closures
	if (non_set.find(non) == non_set.end()) {
		// add configs to closures
		non_set.insert(non);
		for (auto &rule : non->rules) {
			closures.push_back(make_shared<Configuration>(non, rule, 0, lookahead));
		}
	}else {
		// merge lookahead
		for (auto &c : closures) {
			if (c->left_side() != non)
				continue;

			c->lookahead().insert(lookahead.begin(), lookahead.end());
		}
	}
}

shared_ptr<State> State::makeState(
		const vector<shared_ptr<Nonterminal>> &all_symbols,
		const FirstMap &first_map,
		const vector<shared_ptr<Configuration>> &transited_configs) {

	// make nonterminal set to prevent Configuration duplicated
	set<shared_ptr<Nonterminal>> non_set;

	// make closures
	vector<shared_ptr<Configuration>> closures;
	for (auto &c : transited_configs)
		_addClosures(first_map, c, non_set, closures);

	// add closures from closures
	for (int ci = 0; ci < closures.size(); ++ci) {
		auto &c = closures[ci];

		_addClosures(first_map, c, non_set, closures);
	}

	return make_shared<State>(transited_configs, closures);
}

void State::print () {
	cout << "## state " << name_ << endl;
	cout << "transited:" << endl;
	for (auto &c : transited_configs_)
		cout << c->toString();

	cout << "closures:" << endl;
	for (auto &c : closures_)
		cout << c->toString();

	cout << "transition:" << endl;
	for (auto &itr : transition_map_) {
        auto name = (itr.first == null)? "$" : itr.first->name;
		cout << name << " ==> " << itr.second->name() << endl;
	}
}


ParsingTable::ActionInfo::ActionInfo ()
:action(ActionInfo::NONE),
 idx(-1) {
}

ParsingTable::ActionInfo::ActionInfo (Action action, int idx)
:action(action),
 idx(idx) {

}

static void _makeReduceTable (
		const map<const Rule*, int> &rule_idx_map,
		const shared_ptr<Nonterminal> &start_symbol,
		const vector<shared_ptr<Configuration>> &configs,
		map<shared_ptr<TerminalBase>, ParsingTable::ActionInfo> &action_info_map) {

	for (auto &c : configs) {
		auto &rule = c->rule();
		if (c->idx_after_cursor() < rule.right_side.size())
			continue;

		int rule_idx = rule_idx_map.at(&rule);
		for (auto &termnon : c->lookahead()) {
			if (c->left_side() == start_symbol && termnon == null) {
				action_info_map[termnon] = ParsingTable::ActionInfo(
						ParsingTable::ActionInfo::ACCEPT, rule_idx);
			}else {
				action_info_map[termnon] = ParsingTable::ActionInfo(
						ParsingTable::ActionInfo::REDUCE, rule_idx);
			}
		}
	}
}

static void _makeTransitionTable(
		const map<shared_ptr<TerminalBase>, shared_ptr<State>> &transition_map,
		const map<shared_ptr<State>, int> &state_idx_map,
		map<shared_ptr<TerminalBase>, ParsingTable::ActionInfo> &action_info_map) {

	for (auto &itr : transition_map) {
		auto &termnon = itr.first;
		auto &state   = itr.second;

		if (termnon->isTerminal() == true) {
			action_info_map[termnon] =
				ParsingTable::ActionInfo(
					ParsingTable::ActionInfo::SHIFT, state_idx_map.at(state));
		}else {
			action_info_map[termnon] =
				ParsingTable::ActionInfo(
					ParsingTable::ActionInfo::GOTO, state_idx_map.at(state));
		}
	}
}

ParsingTable::ParsingTable(
		const vector<shared_ptr<Nonterminal>> &symbols,
		const shared_ptr<Nonterminal> &start_symbol,
		const vector<shared_ptr<State>> &states) {

    symbols_ = symbols;
    start_symbol_ = start_symbol;

	map<shared_ptr<State>, int> state_idx_map;
	for (int si = 0; si < states.size(); ++si)
		state_idx_map[states[si]] = si;

	// rules
	map<const Rule*, int> rule_idx_map;
	for (auto &r : start_symbol->rules) {
		rule_idx_map[&r] = rules_.size();
		rules_.push_back(&r);
	}
	for (auto &non : symbols) {
		for (auto &r : non->rules) {
			rule_idx_map[&r] = rules_.size();
			rules_.push_back(&r);
		}
	}

    // terminals
	for (auto &rule : rules_) {
        for (auto &termnon : rule->right_side) {
            if (termnon->isTerminal() == false)
                continue;
            auto term = dynamic_pointer_cast<Terminal>(termnon);
            terminal_map_[term->token_type] = term;
        }
	}
    terminal_map_[0] = null;

	// make table
	action_info_map_list_.resize(states.size());
	for (int si = 0; si < states.size(); ++si) {
		auto &s = states[si];
		auto &action_info_map = action_info_map_list_[si];

		// make about reduce
		_makeReduceTable(rule_idx_map, start_symbol, s->transited_configs(), action_info_map);
		_makeReduceTable(rule_idx_map, start_symbol, s->closures()         , action_info_map);

		// transition
		_makeTransitionTable(s->transition_map(), state_idx_map, action_info_map);
	}
}

static string _actionInfoToString (ParsingTable::ActionInfo info) {
    switch (info.action) {
        case ParsingTable::ActionInfo::NONE:
            return "none";
        case ParsingTable::ActionInfo::SHIFT:
            return "s" + to_string(info.idx);
        case ParsingTable::ActionInfo::REDUCE:
            return "r" + to_string(info.idx);
        case ParsingTable::ActionInfo::GOTO:
            return "go" + to_string(info.idx);
        case ParsingTable::ActionInfo::ACCEPT:
            return "acc";
    }
}

static bool _sortFuncForTerminalBaseByString (
        const shared_ptr<TerminalBase> &a,
        const shared_ptr<TerminalBase> &b) {
	if (a == null)
		return true;
	else if (b == null)
		return false;

    return a->name < b->name;
}

string ParsingTable::toString () const {
    stringstream ss;
    ss << "##### Rules" << endl;
    for (int ri=0; ri<rules_.size(); ++ri) {
        auto &r = rules_[ri];
        ss << "# Rule " << ri << " : ";
        ss << r->toString() << endl;
    }


    ss << "##### Table" << endl;
    set<shared_ptr<TerminalBase>> rule_elem_set;
    for (auto &action_map : action_info_map_list_) {
        for (auto &itr : action_map)
            rule_elem_set.insert(itr.first);
    }

    // make field string length map
    map<shared_ptr<TerminalBase>, int> field_str_length_map;
    for (auto &termnon : rule_elem_set)
        field_str_length_map[termnon] = (termnon == null)? 1: termnon->name.size();

    for (auto &action_map : action_info_map_list_) {
        for (auto &itr : action_map) {
            auto &field_str_length = field_str_length_map[itr.first];
            auto action_str = _actionInfoToString(itr.second);
            field_str_length =
                    (action_str.size() > field_str_length)?
                        action_str.size() : field_str_length;
        }
    }


    // print
    vector<shared_ptr<TerminalBase>> rule_elem_vec; // terminal first
    for (auto &termnon : rule_elem_set) {
        if (termnon == null || termnon->isTerminal() == true)
            rule_elem_vec.push_back(termnon);
    }
    sort(rule_elem_vec.begin(), rule_elem_vec.end(), _sortFuncForTerminalBaseByString);

    vector<shared_ptr<TerminalBase>> rule_elem_vec_for_non; // terminal first
    for (auto &termnon : rule_elem_set) {
        if (termnon != null && termnon->isTerminal() == false)
            rule_elem_vec_for_non.push_back(termnon);
    }
    sort(   rule_elem_vec_for_non.begin(),
            rule_elem_vec_for_non.end  (),
            _sortFuncForTerminalBaseByString);
    rule_elem_vec.insert(
            rule_elem_vec.end(),
            rule_elem_vec_for_non.begin(),
            rule_elem_vec_for_non.end());

    
    // field
    int field_length = 0;
    ss << "   " << " | "; field_length += 6;
    for (auto &termnon : rule_elem_vec) {
        auto length = field_str_length_map[termnon];
        ss << setfill(' ') << setw(length) << ((termnon == null)? "$": termnon->name) << " | ";
        field_length += length + 3;
    }
    ss << endl;
    for (int fi=0; fi<field_length; ++fi)
        ss << "-";
    ss << endl;

    // content
    for (int ai=0; ai<action_info_map_list_.size(); ++ai) {
        ss << setfill(' ') << setw(3) << ai << " | ";
        auto &action_map = action_info_map_list_[ai];
        for (auto &termnon : rule_elem_vec) {
            auto length = field_str_length_map[termnon];
            if (action_map.find(termnon) == action_map.end())
                ss << setfill(' ') << setw(length) << " " << " | ";
            else {
                ss << setfill(' ') << setw(length)
                        << _actionInfoToString(action_map.at(termnon)) << " | ";
            }
        }
        ss << endl;
    }

    return ss.str();
}

class NodeStackInfo {
public:
    shared_ptr<Node> node;
    int state_idx;

    NodeStackInfo ()
    :node(null),
     state_idx(-1) {
    }

    NodeStackInfo (const shared_ptr<Node> &node, int state_idx)
    :node(node),
     state_idx(state_idx) {
    }
};

static string _makeErrStringForReduce (
        const Token *t,
        const Rule *rule,
        const vector<NodeStackInfo> &node_stack) {

    stringstream ss;
    ss << "Rule : " << rule->toString() << endl;
    ss << "Nodes : ";
    for (int si=node_stack.size()-rule->right_side.size(); si<node_stack.size(); ++si) {
        cout << node_stack[si].node->termnon()->name << " ";
    }
    ss << endl;
    return ss.str();
}

static bool _reduceStack (
        const vector<map<shared_ptr<TerminalBase>, ParsingTable::ActionInfo>> &action_info_map_list,
        const Token *t,
        const vector<const Rule *> &rules,
        int rule_idx,
        vector<NodeStackInfo> &node_stack) {

    auto rule = rules[rule_idx];

    // check stack size
    if (node_stack.size() < rule->right_side.size()) {
        // TODO err: cannot reduce
        cout << "err: cannot reduce because nodes are not matched with rule." << endl;
        cout << _makeErrStringForReduce(t, rule, node_stack);
        return null;
    }


    // make reduced node
    auto reduced_node = make_shared<Node>(rule->left_side, t);
    reduced_node->reduced_rule_idx(rule_idx);

    for (int ri=0; ri<rule->right_side.size(); ++ri) {
        int si = node_stack.size() - rule->right_side.size() + ri;
        auto &info = node_stack[si];
        auto &termnon = rule->right_side[ri];

        // check node is matched with rule
        if (info.node->termnon() != termnon) {
            //TODO err: cannot reduce
            cout << "err: cannot reduce becuase node\'s type is not matched with rule." << endl;
            cout << _makeErrStringForReduce(t, rule, node_stack);
            return null;
        }

        reduced_node->addChild(info.node);
    }


    // reduce
    node_stack.resize(node_stack.size() - rule->right_side.size());
    auto &last_node = node_stack.back();
    auto &action_info_map = action_info_map_list[last_node.state_idx];
    if (action_info_map.find(reduced_node->termnon()) == action_info_map.end()) {
        // TODO err: cannot reduce
        cout << "err: cannot reduce because no \'go to action\' for \'"
                << reduced_node->termnon()->name << "\' on State " << last_node.state_idx;
        return null;
    }
    auto &action_info = action_info_map.at(reduced_node->termnon());
    if (action_info.action != ParsingTable::ActionInfo::GOTO) {
        // TODO err: cannot reduce
        cout << "err: cannot reduce because no \'go to action\' for \'"
                << reduced_node->termnon()->name << "\' on State " << last_node.state_idx;
        return null;
    }
    node_stack.push_back(NodeStackInfo(reduced_node, action_info.idx));


    return true;
}

static void _printNodeStack (const vector<NodeStackInfo> &node_stack, const char *text) {
    for (auto &nsi : node_stack) {
        if (nsi.node != null)
            cout << nsi.node->toString(text) << " | ";
        cout << nsi.state_idx << " | ";
    }
    cout << endl;
}

shared_ptr<Node> ParsingTable::generateParseTree (
        const char *text,
        const vector<Token> &tokens,
        bool need_print) {
    vector<NodeStackInfo> node_stack;
    node_stack.push_back(NodeStackInfo(null, 0));

    for (int ti=0; ti<tokens.size(); ) {
        auto &t = tokens[ti];

        // get terminal for token
        if (terminal_map_.find(t.type) == terminal_map_.end()) {
            // TODO err: token {t.type} cannot be parsed
            cout << "err: token " << t.type << " cannot be parsed" << endl;
            return null;
        }
        auto &term = terminal_map_[t.type];


        // check stack and action map
        auto &nsi = node_stack.back();
        auto &action_info_map = action_info_map_list_[nsi.state_idx];

        if (action_info_map.find(term) == action_info_map.end()) {
            // TODO err: cannot be parsed on {t.first_idx~t.last_idx}
            cout << "err: cannot be parsed \""
                    << t.toString(text)
                    << "\" State " << nsi.state_idx << " idx:" << t.first_idx << endl;
            return null;
        }

        auto &action_info = action_info_map[term];
        switch (action_info.action) {
            case ActionInfo::Action::SHIFT:
                if (need_print == true)
                    cout << "shift " << action_info.idx << " with " << t.toString(text) << endl;
                node_stack.push_back(NodeStackInfo(make_shared<Node>(term, &t), action_info.idx));
                ++ti;
                break;
            case ActionInfo::Action::REDUCE:
                if (need_print == true) {
                    cout << "reduce " << action_info.idx
                            << " with " << t.toString(text)
                            << " #Rule : " << rules_[action_info.idx]->toString() << endl;
                }
                _reduceStack(action_info_map_list_, &t, rules_, action_info.idx, node_stack);
                break;
            case ActionInfo::Action::ACCEPT:
                if (need_print == true)
                    cout << "accept" << " with " << t.toString(text) << endl;
                return node_stack.back().node;
            default:
                // TODO err: unknown action \'{action_info.action}\'
                cout << "unknown action \'" << action_info.action << "\'" << endl;
                return null;
        }

        if (need_print == true)
            _printNodeStack(node_stack, text);
    }

    // TODO err: cannot reduce. syntax error.
    cout << "err: cannot reduce. syntax error." << endl;
    _printNodeStack(node_stack, text);

    return null;
}

static void _loadNonterminal(
		const PawPrint::Cursor &pp_rules,
		const shared_ptr<Nonterminal> &non,
		unordered_map<string, shared_ptr<TerminalBase>> termnon_map) {

	non->rules.resize(pp_rules.size());
	for (int ri = 0; ri<pp_rules.size(); ++ri) {
		auto pp_rule = pp_rules[ri];

		auto &rule = non->rules[ri];
		rule.left_side = dynamic_pointer_cast<Nonterminal>(
			termnon_map[pp_rule[0].get("")]);

		rule.right_side.resize(pp_rule.size() - 1);
		for (int rri = 1; rri<pp_rule.size(); ++rri)
			rule.right_side[rri - 1] = termnon_map[pp_rule[rri].get("")];
	}
}

ParsingTable::ParsingTable (const vector<unsigned char> &data) {

    PawPrint paw(data);
    auto root = paw.root();

    unordered_map<string, shared_ptr<TerminalBase>> termnon_map;

    // load terminals
    auto pp_terminals = root[0];
    for (int ti=0; ti<pp_terminals.size(); ++ti) {
        auto pp_term = pp_terminals[ti];
        auto name       = pp_term[0].get("");
        auto token_type = pp_term[1].get(-1);

        auto term = make_shared<Terminal>(name, token_type);
		if (strcmp(name, "$") == 0)
			term = null;
        terminal_map_[token_type] = term;
        termnon_map[name] = term;
    }

    // load nonterminals
    auto pp_nonterminals = root[1];
    for (int ni=0; ni<pp_nonterminals.size(); ni+=2) {
        auto name = pp_nonterminals[ni].get("");

        auto non = make_shared<Nonterminal>(name);
        termnon_map[name] = non;
		symbols_.push_back(non);
    }
    for (int ni=0; ni<pp_nonterminals.size(); ni+=2) {
        auto name     = pp_nonterminals[ni  ].get("");
        auto pp_rules = pp_nonterminals[ni+1];

        auto non = dynamic_pointer_cast<Nonterminal>(termnon_map[name]);
		_loadNonterminal(pp_rules, non, termnon_map);
    }

    // start symbol
	auto start_symbol_name     = root[2].get("");
	auto pp_start_symbol_rules = root[4];
	start_symbol_ = make_shared<Nonterminal>(start_symbol_name);
	_loadNonterminal(pp_start_symbol_rules, start_symbol_, termnon_map);
	for (auto &r : start_symbol_->rules)
		r.left_side = start_symbol_;

    // action_info_map_list_
    auto pp_action_info_map_list = root[5];
    action_info_map_list_.resize(pp_action_info_map_list.size());
    for (int aim_idx=0; aim_idx<pp_action_info_map_list.size(); ++aim_idx) {
        auto pp_action_info_map = pp_action_info_map_list[aim_idx];
        auto &action_info_map   = action_info_map_list_  [aim_idx];
        for (int ai_idx=0; ai_idx<pp_action_info_map.size(); ++ai_idx) {
            auto name = pp_action_info_map.getKey(ai_idx);
            auto pp_action_info = pp_action_info_map[ai_idx];

            auto &termnon = termnon_map[name];
            action_info_map[termnon] = ActionInfo(
                    (ParsingTable::ActionInfo::Action)pp_action_info[0].get(-1),
                    pp_action_info[1].get(-1));
        }
    }

	// rules
	map<const Rule*, int> rule_idx_map;
	for (auto &r : start_symbol_->rules) {
		rule_idx_map[&r] = rules_.size();
		rules_.push_back(&r);
	}
	for (auto &non : symbols_) {
		for (auto &r : non->rules) {
			rule_idx_map[&r] = rules_.size();
			rules_.push_back(&r);
		}
	}
}

static void _pushNonterminal (const shared_ptr<Nonterminal> &non, PawPrint &paw) {
	paw.pushString(non->name);
	paw.beginSequence(); // rules
	for (auto &r : non->rules) {
		paw.beginSequence();
            paw.pushString(r.left_side->name);
            for (auto &rn : r.right_side) {
                paw.pushString(rn->name);
            }
		paw.endSequence();
	}
	paw.endSequence();
}

bool ParsingTable::saveBinary (vector<unsigned char> &result) {
    PawPrint paw;

    paw.beginSequence();
    
        // terminals
        paw.beginSequence();
            for (auto &itr : terminal_map_) {
                auto &term = itr.second;
				paw.beginSequence();
				if (term != null) {
					paw.pushString(term->name);
					paw.pushInt(term->token_type);
				}else {
					paw.pushString("$");
					paw.pushInt(0);
				}
				paw.endSequence();
            }
        paw.endSequence();

        // nonterminals
        paw.beginSequence();
            for (auto &non : symbols_)
				_pushNonterminal(non, paw);
        paw.endSequence();

        // start symbol
		_pushNonterminal(start_symbol_, paw);

        // action_info_map_list_
        paw.beginSequence();
            for (auto &action_info_map : action_info_map_list_) {
                paw.beginMap();
                    for (auto &itr : action_info_map) {
                        auto &termnon = itr.first;
                        auto &info    = itr.second;

						if (termnon != null)
							paw.pushKey(termnon->name);
						else
							paw.pushKey("$");
                        paw.beginSequence(); // ActionInfo
                            paw.pushInt(info.action);
                            paw.pushInt(info.idx   );
                        paw.endSequence();
                    }
                paw.endMap();
            }
        paw.endSequence();

    paw.endSequence();

    result = paw.raw_data();

    return true;
}

}

