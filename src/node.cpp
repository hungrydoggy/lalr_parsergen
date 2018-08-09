#include "node.h"

#include "./defines.h"

#include <iostream>
#include <set>
#include <sstream>


namespace parse_table {

using std::cout;
using std::endl;
using std::dynamic_pointer_cast;
using std::set;
using std::stringstream;

using RuleHistoryByTokenIdx = unordered_map<int, set<unsigned int>>;


shared_ptr<Nonterminal> TerminalBase::start_;

unsigned int Nonterminal::next_no_ = 0;



Rule::Rule (
        const shared_ptr<Nonterminal> &left_side,
        const vector<shared_ptr<TerminalBase>> &right_side)
:left_side(left_side),
 right_side(right_side) {
}

string Rule::toString () const {
    stringstream ss;
    ss << left_side->name << " -> ";
    for (auto &termnon : right_side) {
        ss << termnon->name << " ";
    }

    return ss.str();
}

TerminalBase::TerminalBase (const string &name)
:name(name) {
}

Terminal::Terminal (const string &name, int token_type)
:TerminalBase(name),
 token_type(token_type) {
}

Nonterminal::Nonterminal (const string &name)
:TerminalBase(name) {
	no_ = next_no_;
	++next_no_;
}

Node::Node (const shared_ptr<TerminalBase> &termnon, const Token *token)
:parent_(null),
 termnon_(termnon),
 token_(token),
 rule_(null) {

}

void Node::addChild (const shared_ptr<Node> &child) {
    children_.push_back(child);
    child->parent_ = this;
}

string Node::toString (const char *text, int indent, bool with_children, int indent_inc) {
    if (indent_inc < 2)
        indent_inc = 2;

    stringstream ss;

    // print indent
    if (indent > 0) {
        for (int i=0; i<indent; ++i)
            ss << (((i%indent_inc) == 0)? "|": "-");
    }

    // print self
    if (termnon_->isTerminal() == true)
        ss << "Terminal(\"" << termnon_->name << "\", " << token_->toString(text) << ")";
    else
        ss << "Nonterminal(\"" << termnon_->name << "\")";

    // print children
    if (with_children == true) {
        for (auto &c : children_)
            ss << endl << c->toString(text, indent + indent_inc, indent_inc);
    }

    return ss.str();
}

}
