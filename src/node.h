#ifndef PAW_PRINT_NODE
#define PAW_PRINT_NODE

#include <memory>
#include <unordered_map>
#include <vector>

#include "./token.h"

#include "./defines.h"

namespace parse_table {

using std::make_shared;
using std::shared_ptr;
using std::unordered_map;
using std::vector;

class Node;
class Nonterminal;


class PAW_PRINT_API TerminalBase {
public:
    string name;

    TerminalBase (const string &name);
    virtual ~TerminalBase () {}

    virtual bool isTerminal () = 0;

};

class PAW_PRINT_API Rule {
public:
	shared_ptr<Nonterminal> left_side;
	vector<shared_ptr<TerminalBase>> right_side;

    Rule () {}

	Rule (  const shared_ptr<Nonterminal> &left_side,
            const vector<shared_ptr<TerminalBase>> &right_side);

    string toString () const;
};

class PAW_PRINT_API Terminal : public TerminalBase {
public:
    int token_type;

    Terminal (const string &name, int token_type);

    bool isTerminal () override { return true; }
};

class PAW_PRINT_API Nonterminal : public TerminalBase {
public:
    vector<Rule> rules;

	//inline unsigned int no () { return no_; }


    Nonterminal (const string &name);

    bool isTerminal () override { return false; }


private:
	//static unsigned int next_no_;
	
	//unsigned int no_;
};


class PAW_PRINT_API Node {
public:
    PAW_GETTER(Node*, parent)
    PAW_GETTER(const shared_ptr<TerminalBase>&, termnon)
    PAW_GETTER(const Token*, token)
    PAW_GETTER_SETTER(int, reduced_rule_idx)
    PAW_GETTER(const vector<shared_ptr<Node>>&, children)

    Node (const shared_ptr<TerminalBase>& termnon, const Token *token);

    void addChild (const shared_ptr<Node> &child);

    string toString (const char *text, int indent=0, bool with_children=false, int indent_inc=2);

private:
    Node *parent_;
    shared_ptr<TerminalBase> termnon_;
    const Token *token_;
    int reduced_rule_idx_;
    vector<shared_ptr<Node>> children_;
};

}

#include "./undefines.h"
#endif
