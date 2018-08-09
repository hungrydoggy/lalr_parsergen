#include "./parsing_table_generator.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>


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


ParsingTableGenerator::ParsingTableGenerator () {
}

void ParsingTableGenerator::addSymbol(const shared_ptr<Nonterminal> &non, bool is_start_symbol/*=false*/) {
	symbols_.push_back(non);

	if (is_start_symbol == true)
		start_symbol_ = non;
}

static void _makeNextTransitionInfoMap (
		const vector<shared_ptr<Configuration>> &configs,
		map<shared_ptr<TerminalBase>, vector<shared_ptr<Configuration>>> &result) {

	for (auto &c : configs) {
		if (c->idx_after_cursor() >= c->rule().right_side.size())
			continue;

		auto &termnon = c->rule().right_side[c->idx_after_cursor()];

		result[termnon].push_back(
				make_shared<Configuration>(
					c->left_side(),
					c->rule(),
					c->idx_after_cursor() + 1,
					c->lookahead()));
	}
}

static bool _areConfigsEqual (
        const shared_ptr<Configuration> &config,
        const shared_ptr<Configuration> &other,
        bool need_check_lookahead) {

    if (config->left_side()              != other->left_side()             ||
        config->idx_after_cursor()       != other->idx_after_cursor()      ||
        config->rule().right_side.size() != other->rule().right_side.size())
        return false;

    if (need_check_lookahead == true && config->lookahead() != other->lookahead())
        return false;

    auto &config_r = config->rule();
    auto &other_r  = other->rule();
    for (int ri = 0; ri < config_r.right_side.size(); ++ri) {
        if (config_r.right_side[ri] != other_r.right_side[ri])
            return false;
    }

    return true;
}

static bool _areStatesEqual (
		const shared_ptr<State> &state,
		const shared_ptr<State> &other,
        bool need_check_lookahead) {

	if (state->transited_configs().size() != other->transited_configs().size())
		return false;

	if (state->closures().size() != other->closures().size())
		return false;

	for (int ci = 0; ci < state->transited_configs().size(); ++ci) {
		auto &state_c = state->transited_configs()[ci];
		auto &other_c = other->transited_configs()[ci];
        if (_areConfigsEqual(state_c, other_c, need_check_lookahead) == false)
            return false;
	}

	return true;
}

static void _mergeStates_configs(
		const vector<shared_ptr<Configuration>> &configs,
		const vector<shared_ptr<Configuration>> &other) {
	for (int ci = 0; ci < configs.size(); ++ci) {
		auto &c       = configs[ci];
		auto &other_c = other  [ci];
		c->lookahead().insert(other_c->lookahead().begin(), other_c->lookahead().end());
	}
}

static void _mergeStates (
		vector<shared_ptr<State>> &states,
		vector<shared_ptr<State>> &result) {

	vector<shared_ptr<State>> new_states;
	// merge states
	for (int si = 0; si < states.size(); ++si) {
		auto &s = states[si];
		if (s == null)
			continue;

		new_states.push_back(s);

		for (int other_i = si + 1; other_i < states.size(); ++other_i) {
			auto &other_s = states[other_i];
			if (other_s == null)
				continue;

			if (_areStatesEqual(s, other_s, false) == false)
				continue;


			// merge
			_mergeStates_configs(s->transited_configs(), other_s->transited_configs());
			_mergeStates_configs(s->closures()         , other_s->closures()         );

			for (auto &sss : states) {
				if (sss == null)
					continue;

				auto &transition_map = sss->transition_map();
				for (auto &itr : transition_map) {
					if (itr.second == other_s)
						transition_map[itr.first] = s;
				}
			}

			states[other_i] = null;
		}
	}

	result = new_states;
}

static shared_ptr<State> _findState(
		const vector<shared_ptr<State>> &states,
		shared_ptr<State> new_state) {
	
	for (int si = 0; si < states.size(); ++si) {
		auto &s = states[si];
		if (s == null)
			continue;

		if (_areStatesEqual(s, new_state, true) == true)
			return s;
	}

	return null;
}

static void _findFirst (
		const shared_ptr<TerminalBase> &termnon,
		set<shared_ptr<TerminalBase>> &result,
		set<shared_ptr<TerminalBase>> &history) {
	
	if (termnon->isTerminal()) {
		result.insert(termnon);
		return;
	}


	// nonterminal part
	auto non = dynamic_pointer_cast<Nonterminal>(termnon);

	// prevent infinity loop
	if (history.find(termnon) != history.end())
		return;
	history.insert(termnon);

	// find recursively
	for (auto rule : non->rules) {
		auto &first_re = rule.right_side[0];
		_findFirst(first_re, result, history);
	}
}

static void _findFirst(
		const shared_ptr<TerminalBase> &termnon,
		set<shared_ptr<TerminalBase>> &result) {

	set<shared_ptr<TerminalBase>> history;
	return _findFirst(termnon, result, history);
}

static void _addStates(
	const vector<shared_ptr<Nonterminal>> &symbols,
	const FirstMap &first_map,
	shared_ptr<State> base,
	vector<shared_ptr<State>> &all_states,
	set<State*> &history) {

	if (history.find(base.get()) != history.end())
		return;
	history.insert(base.get());

	// make next_map
	map<shared_ptr<TerminalBase>, vector<shared_ptr<Configuration>>> next_map;
	_makeNextTransitionInfoMap(base->transited_configs(), next_map);
	_makeNextTransitionInfoMap(base->closures(), next_map);

	// make new state
	for (auto &itr : next_map) {
		auto new_state = State::makeState(symbols, first_map, itr.second);

		auto old_one = _findState(all_states, new_state);
		if (old_one != null) {
			base->transition_map()[itr.first] = old_one;
		}else {
			base->transition_map()[itr.first] = new_state;
			all_states.push_back(new_state);
		}
	}

}

shared_ptr<ParsingTable> ParsingTableGenerator::generateTable () {

	if (start_symbol_ == null) {
		//TODO err: you have to set start_symbol
		return null;
	}

    // make rule elements
    rule_elements_.clear();
    for (auto &non :  symbols_) {
        rule_elements_.insert(non);
        for (auto &rule : non->rules) {
            for (auto &termnon : rule.right_side)
                rule_elements_.insert(termnon);
        }
    }

	// make first map
	FirstMap first_map;
	for (auto &termnon : rule_elements_) {
		auto &first = first_map[termnon];
		_findFirst(termnon, first);
	}


	// make s_prime
	auto s_prime = make_shared<Nonterminal>("S\'");
	s_prime->rules.push_back(Rule(s_prime, { start_symbol_ }));

	// make start state
	set<shared_ptr<TerminalBase>> start_lookahead{ null };
	vector<shared_ptr<Configuration>> start_configs {
		make_shared<Configuration>(s_prime, s_prime->rules[0], 0, start_lookahead)
	};

	auto start_state = State::makeState(symbols_, first_map, start_configs);
	states_.push_back(start_state);

	// add states
	set<State*> history;
	for (int si = 0; si < states_.size(); ++si) {
		auto &s = states_[si];
		s->name("State " + to_string(si));
		_addStates(symbols_, first_map, s, states_, history);
	}

	// merge states
	_mergeStates(states_, states_);

	/*// print states
	for (auto &s : states_) {
		cout << "################################" << endl;
		s->print();
	}*/

	// make parsing table
	return make_shared<ParsingTable>(symbols_, s_prime, states_);
}

}
