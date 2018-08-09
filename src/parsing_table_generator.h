#ifndef PARSING_TABLE_GENERATOR
#define PARSING_TABLE_GENERATOR

#include "./parse_table.h"

#include "./defines.h"


namespace parse_table {

class ParsingTableGenerator {
public:
	PAW_GETTER(const shared_ptr<Nonterminal>&, start_symbol)

	ParsingTableGenerator ();

	void addSymbol (const shared_ptr<Nonterminal> &non, bool is_start_symbol = false);

	shared_ptr<ParsingTable> generateTable ();

private:
	vector<shared_ptr<Nonterminal>> symbols_;
	shared_ptr<Nonterminal> start_symbol_;
	vector<shared_ptr<State>> states_;
    set<shared_ptr<TerminalBase>> rule_elements_;
};

}

#include "./undefines.h"

#endif
