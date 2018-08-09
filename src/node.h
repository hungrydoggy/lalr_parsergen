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


class TerminalBase {
public:
    inline static shared_ptr<Nonterminal> start () {
        _init();
        return start_;
    };
    

    string name;

    TerminalBase (const string &name);
    virtual ~TerminalBase () {}

    virtual bool isTerminal () = 0;

private:
    static shared_ptr<Nonterminal> start_;
    static void _init ();

};

class Rule {
public:
	shared_ptr<Nonterminal> left_side;
	vector<shared_ptr<TerminalBase>> right_side;

	Rule (  const shared_ptr<Nonterminal> &left_side,
            const vector<shared_ptr<TerminalBase>> &right_side);

    string toString () const;
};

class Terminal : public TerminalBase {
public:
    int token_type;

    Terminal (const string &name, int token_type);

    bool isTerminal () override { return true; }
};

class Nonterminal : public TerminalBase {
public:
    vector<Rule> rules;

	inline unsigned int no () { return no_; }


    Nonterminal (const string &name);

    bool isTerminal () override { return false; }


private:
	static unsigned int next_no_;
	
	unsigned int no_;
};


class Node {
public:
    PAW_GETTER(Node*, parent)
    PAW_GETTER(const shared_ptr<TerminalBase>&, termnon)
    PAW_GETTER(const Token*, token)
    PAW_GETTER_SETTER(const Rule*, rule)
    PAW_GETTER(const vector<shared_ptr<Node>>&, children)

    Node (const shared_ptr<TerminalBase>& termnon, const Token *token);

    void addChild (const shared_ptr<Node> &child);

    string toString (const char *text, int indent=0, bool with_children=false, int indent_inc=2);

private:
    Node *parent_;
    shared_ptr<TerminalBase> termnon_;
    const Token *token_;
    const Rule *rule_;
    vector<shared_ptr<Node>> children_;
};

}

#include "./undefines.h"
#endif
