#include "syntax.hpp"
#include "lexer.hpp"
#include <cassert>
#include <cmath>
#include <memory>
#include <queue>
#include <set>
#include <stack>
#include <tuple>
#include <unordered_set>

using namespace pl0cc;

namespace {
    template <typename T>
    void mergeSet(std::set<T>& s1, const std::set<T>& s2) {
        s1.insert(s2.begin(), s2.end());
    }

    template <typename T>
    bool mergeSetCheck(std::set<T>& s1, const std::set<T>& s2) {
        bool changed = false;
        for (const T& v : s2) {
            if (!s1.count(v)) {
                changed = true;
                s1.insert(v);
            }
        }
        return changed;
    }
}

void Syntax::addConduct(Symbol leftPart, Sentence rightPart) {
    firstSetsValid = false;
    emptySymbolSetValid = false;

    addSymbol(leftPart);
    ntSymbolSet.insert(leftPart);
    for (Symbol s : rightPart) {
        addSymbol(s);
    }
    if (!sentences[leftPart].count(rightPart)) {
        conductVector.emplace_back(leftPart, rightPart);
        sentences[leftPart].insert(rightPart);
    }
}

const std::set<Symbol>& Syntax::firstSet(Symbol s) const {
    if (!firstSetsValid) {
        calculateFirstSet();
    }
    return firstSets[s];
}

std::set<Symbol> Syntax::firstSet(const Sentence& stmt) const {
	std::set<Symbol> first{EPS};

	for (size_t i = 0; i < stmt.size() && first.count(EPS); i++) {
        first.erase(EPS);
        if (ntSymbolSet.count(stmt[i])) {
		    mergeSet(first, firstSet(stmt[i]));
        } else {
            first.insert(stmt[i]);
        }
	}

	return first;
}

const std::set<Symbol>& Syntax::followSet(Symbol s) const {
    if (!followSetsValid) calculateFollowSet();
    return followSets[s];
}

std::set<Symbol> Syntax::selectSet(Symbol leftPart, const Sentence& rightPart) const {
    std::set<Symbol> symbols = firstSet(rightPart);
    if (symbols.count(EPS)) {
        symbols.erase(EPS);
        mergeSet(symbols, followSet(leftPart));
    }
    return symbols;
}

std::map<Symbol, std::map<Symbol, Sentence>> Syntax::llMap() const {
    std::map<Symbol, std::map<Symbol, Sentence>> selectMap;
    for (auto [conductLeft, conductRight] : conducts()) {
        auto nextSymbols = selectSet(conductLeft, conductRight);
        for (auto sym : nextSymbols) {
            selectMap[conductLeft][sym] = conductRight;
        }
    }
    return selectMap;
}

Symbol Syntax::start() const {
    return startSymbol;
}

const std::set<Symbol>& Syntax::symbols() const {
    return symbolSet;
}

const std::set<Symbol>& Syntax::nonTerminatingSymbols() const {
    return ntSymbolSet;
}

const std::vector<std::pair<Symbol, Sentence>>& Syntax::conducts() const {
    return conductVector;
}

Symbol Syntax::addSymbol(Symbol sym) {
    symbolSet.insert(sym);
    return sym;
}

void Syntax::searchEmptySymbols() const {
    bool updated;
    emptySymbolSet.clear();

    do {
        updated = false;
        for (Symbol s : ntSymbolSet) {
            if (emptySymbolSet.count(s)) {
                continue;
            }
            if (sentences.at(s).count(Sentence())) {
                emptySymbolSet.insert(s);
                updated = true;
                continue;
            }
            for (const Sentence& st : sentences.at(s)) {
                bool fullEmpty = true;
                // This loop checks whether the Sentence st contains non-empty symbol
                for (Symbol sym : st) {
                    if (!emptySymbolSet.count(sym)) {
                        fullEmpty = false;
                        break;
                    }
                }
                // If the sentence st doesn't contain non-empty symbol, then the symbol s can be conducted to empty and it is an empty symbol
                if (fullEmpty) {
                    emptySymbolSet.insert(s);
                    updated = true;
                    break;
                }
            }
        }
    } while (updated);

    emptySymbolSetValid = true;
}

void Syntax::calculateFirstSet() const {
    firstSets.clear();

    if (!emptySymbolSetValid) searchEmptySymbols();

    std::set<Symbol> searchPath;
    for (Symbol s : ntSymbolSet) { firstSetFor(s, searchPath, firstSets); }

    firstSetsValid = true;
}

std::set<Symbol> Syntax::firstSetFor(Symbol s, std::set<Symbol>& searchPath, std::map<Symbol, std::set<Symbol>>& storage) const {
    if (!ntSymbolSet.count(s)) return {s};

    if (storage.count(s)) return storage[s];
    
    storage[s] = {};
    std::set<Symbol>& ans = storage[s];

    bool isEmptySymbol = emptySymbolSet.count(s);
    if (isEmptySymbol) {
        ans.insert(EPS);
    }

    searchPath.insert(s);

    const std::unordered_set<Sentence>& conductSentences = sentences.at(s);
    /*
    if (conductSentences.count(Sentence())) {
        ans.insert(EPS);
    }
    */

    for (const Sentence& con : conductSentences) {
        if (con.size() == 0) {
            continue;
        }

        if (!ntSymbolSet.count(con[0])) {
            ans.insert(con[0]);
            continue;
        }

        size_t counter = 0;

        std::set<Symbol> secondAns{EPS};
        while (counter < con.size() && secondAns.count(EPS)) {
            secondAns.erase(EPS);
            if (searchPath.count(con[counter]) && !emptySymbolSet.count(con[counter])) break;
            while (counter < con.size() && searchPath.count(con[counter])) {
                counter++;
            }
            if (counter == con.size()) break;
            mergeSet(secondAns, firstSetFor(con[counter++], searchPath, storage));
        }
        ans.insert(secondAns.begin(), secondAns.end());
    }

    searchPath.erase(s);
    return ans;
}

void Syntax::calculateFollowSet() const {
    if (!firstSetsValid) calculateFirstSet();

    followSets.clear();

    bool changed;
    followSets[startSymbol].insert(EPS);

    do {
        changed = false;
        std::set<Symbol> searched;
        std::queue<Symbol> searchQueue;
        searchQueue.push(startSymbol);
        searched.insert(startSymbol);

        while (!searchQueue.empty()) {
            Symbol s = searchQueue.front();
            searchQueue.pop();

            for (const Sentence& stmt : sentences.at(s)) {
                for (size_t idx = 0; idx < stmt.size(); idx++) {
                    Symbol s2 = stmt[idx];
                    if (!ntSymbolSet.count(s2)) {
                        continue;
                    }
                    if (!searched.count(s2)) {
                        searched.insert(s2);
                        searchQueue.push(s2);
                    }
                    if (idx < stmt.size() - 1) {
                        auto rightPartFirst = firstSet(stmt.substr(idx+1));
                        if (rightPartFirst.count(EPS)) {
                            changed |= mergeSetCheck(followSets[s2], followSets[s]);
                            rightPartFirst.erase(EPS);
                        }
                        changed |= mergeSetCheck(followSets[s2], rightPartFirst);
                    } else {
                        changed |= mergeSetCheck(followSets[s2], followSets[s]);
                    }
                }
            }
        }
    } while (changed);

    followSetsValid = true;
}


SyntaxTree::SyntaxTree(Token token) : symbolData(Symbol(token.type)), tokenData(token), childs() {}

SyntaxTree::SyntaxTree(Symbol symbol) : symbolData(symbol), tokenData(), childs() {}

Symbol SyntaxTree::symbol() const {
    return symbolData;
}

void SyntaxTree::addChild(SyntaxTree st) {
    childs.push_back(std::make_shared<SyntaxTree>(std::move(st)));
}

size_t SyntaxTree::childCount() const {
    return childs.size();
}

bool SyntaxTree::childExists(size_t index) const {
    return index >= 0 && index < childs.size() && childs[index] != nullptr;
}

const SyntaxTree& SyntaxTree::childAt(size_t index) const {
    return *childs[index];
}

SyntaxTree& SyntaxTree::childAt(size_t index) {
    return *childs[index];
}

std::shared_ptr<SyntaxTree> SyntaxTree::shareChild(size_t index) {
    return childs[index];
}

void SyntaxTree::setChildSentence(const Sentence& sentence) {
    for (auto sym : sentence) {
        addChild(SyntaxTree(sym));
    }
}

void SyntaxTree::setTokenData(Token token) {
    tokenData = token;
}


void SyntaxTree::serializeTo(std::ostream& os, std::function<std::string(Symbol)> symbolName, int tabCount) {
    for (int i=0; i<tabCount; i++) {
        os << "|";
    }
    os << symbolName(symbolData);
    if (tokenData.has_value()) {
        os << " with token seman " << tokenData->seman;
    }
    os << '\n';

    for (auto ch : childs) {
        if (ch == nullptr) continue;
        ch->serializeTo(os, symbolName, tabCount+1);
    }
}

Syntax pl0cc::genSyntax() {
    using namespace pl0cc::symbols;

    Syntax syn(PROGRAM);
    
    syn.addConduct(LITERAL, {Symbol(TokenType::NUMBER)});
    syn.addConduct(LITERAL, {Symbol(TokenType::STRING)});
    syn.addConduct(UNARY_OP, {Symbol(TokenType::OP_NOT)});
    syn.addConduct(UNARY_OP, {Symbol(TokenType::OP_SUB)});
    syn.addConduct(UNARY_OP, {Symbol(TokenType::OP_PLUS)});
    syn.addConduct(BI_OP4, {Symbol(TokenType::OP_MUL)});
    syn.addConduct(BI_OP4, {Symbol(TokenType::OP_DIV)});
    syn.addConduct(BI_OP4, {Symbol(TokenType::OP_MOD)});
    syn.addConduct(BI_OP3, {Symbol(TokenType::OP_PLUS)});
    syn.addConduct(BI_OP3, {Symbol(TokenType::OP_SUB)});
    syn.addConduct(BI_OP2, {Symbol(TokenType::OP_GT)});
    syn.addConduct(BI_OP2, {Symbol(TokenType::OP_GE)});
    syn.addConduct(BI_OP2, {Symbol(TokenType::OP_LT)});
    syn.addConduct(BI_OP2, {Symbol(TokenType::OP_LE)});
    syn.addConduct(BI_OP2, {Symbol(TokenType::OP_NEQ)});
    syn.addConduct(BI_OP2, {Symbol(TokenType::OP_EQU)});
    syn.addConduct(BI_OP1, {Symbol(TokenType::OP_AND)});
    syn.addConduct(BI_OP1, {Symbol(TokenType::OP_OR)});
    syn.addConduct(TYPE, {Symbol(TokenType::INT)});
    syn.addConduct(TYPE, {Symbol(TokenType::FLOAT)});
    syn.addConduct(TYPE, {Symbol(TokenType::CHAR)});
    
    syn.addConduct(SINGLE_EXPR, {LITERAL});
    syn.addConduct(SINGLE_EXPR, {SYM_OR_FCAL});
    syn.addConduct(SINGLE_EXPR, {Symbol(TokenType::LSBRACKET), EXPR, Symbol(TokenType::RSBRACKET)});

    syn.addConduct(L5_EXPR, {SINGLE_EXPR});
    syn.addConduct(L5_EXPR, {UNARY_OP, SINGLE_EXPR});

    syn.addConduct(L4_EXPR_P, {});
    syn.addConduct(L4_EXPR_P, {BI_OP4, L4_EXPR});
    syn.addConduct(L4_EXPR, {L5_EXPR, L4_EXPR_P});

    syn.addConduct(L3_EXPR_P, {});
    syn.addConduct(L3_EXPR_P, {BI_OP3, L3_EXPR});
    syn.addConduct(L3_EXPR, {L4_EXPR, L3_EXPR_P});

    syn.addConduct(L2_EXPR_P, {});
    syn.addConduct(L2_EXPR_P, {BI_OP2, L2_EXPR});
    syn.addConduct(L2_EXPR, {L3_EXPR, L2_EXPR_P});

    syn.addConduct(L1_EXPR_P, {});
    syn.addConduct(L1_EXPR_P, {BI_OP1, L1_EXPR});
    syn.addConduct(L1_EXPR, {L2_EXPR, L1_EXPR_P});

    syn.addConduct(EXPR, {L1_EXPR});

    syn.addConduct(SYM_OR_FCAL, {Symbol(TokenType::SYMBOL), ARGS_E});

    syn.addConduct(ARGS_E, {});
    syn.addConduct(ARGS_E, {Symbol(TokenType::LSBRACKET), COMMA_SEP_E, Symbol(TokenType::RSBRACKET)});

    syn.addConduct(COMMA_SEP_E, {});
    syn.addConduct(COMMA_SEP_E, {COMMA_SEP});

    syn.addConduct(COMMA_SEP, {EXPR, COMMA_SEP_P});

    syn.addConduct(COMMA_SEP_P, {Symbol(TokenType::COMMA), COMMA_SEP});

    syn.addConduct(VARDEF, {TYPE, Symbol(TokenType::SYMBOL)});

    syn.addConduct(STMT, {VARDEF, Symbol(TokenType::SEMICOLON)});
    syn.addConduct(STMT, {Symbol(TokenType::SYMBOL), Symbol(TokenType::ASSIGN), EXPR, Symbol(TokenType::SEMICOLON)});
    syn.addConduct(STMT, {Symbol(TokenType::LLBRACKET), STMTS, Symbol(TokenType::RLBRACKET)});
    syn.addConduct(STMT, {IFSTMT});
    syn.addConduct(STMT, {WHILESTMT});
    syn.addConduct(STMT, {Symbol(TokenType::RETURN), EXPR, Symbol(TokenType::SEMICOLON)});
    syn.addConduct(STMT, {Symbol(TokenType::BREAK), Symbol(TokenType::SEMICOLON)});
    syn.addConduct(STMT, {Symbol(TokenType::CONTINUE), Symbol(TokenType::SEMICOLON)});
    
    syn.addConduct(STMTS, {});
    syn.addConduct(STMTS, {STMT, STMTS});

    syn.addConduct(IFSTMT, {Symbol(TokenType::IF), Symbol(TokenType::LSBRACKET), EXPR, Symbol(TokenType::RSBRACKET), STMT, ELSECLAUSE});
    syn.addConduct(ELSECLAUSE, {});
    syn.addConduct(ELSECLAUSE, {Symbol(TokenType::ELSE), STMT});

    syn.addConduct(WHILESTMT, {Symbol(TokenType::WHILE), Symbol(TokenType::LSBRACKET), EXPR, Symbol(TokenType::RSBRACKET), STMT});

    syn.addConduct(FNDEF, {Symbol(TokenType::FN), Symbol(TokenType::SYMBOL), Symbol(TokenType::LSBRACKET), VIRTVARDEFS, Symbol(TokenType::RSBRACKET), Symbol(TokenType::ARROW), TYPE, STMT});
    syn.addConduct(VIRTVARDEFS, {VARDEF, VIRTVARDEFS_P});
    syn.addConduct(VIRTVARDEFS_P, {});
    syn.addConduct(VIRTVARDEFS_P, {Symbol(TokenType::COMMA), VIRTVARDEFS});
    syn.addConduct(PROGRAM_PART, {VARDEF, Symbol(TokenType::COMMA)});
    syn.addConduct(PROGRAM_PART, {FNDEF});
    syn.addConduct(PROGRAM, {});
    syn.addConduct(PROGRAM, {PROGRAM_PART, PROGRAM});
    return syn;
}

const std::map<Symbol, std::string>& pl0cc::symbols::symbolToNameMap() {
    static std::map<Symbol, std::string> smap;
    static bool edited = false;

    if (edited) {
        return smap;
    }
#define SYMDEF(name, value) smap[value] = #name

        SYMDEF(LITERAL,       256);
        SYMDEF(SINGLE_EXPR,   257);
        SYMDEF(L5_EXPR,       258);
        SYMDEF(L4_EXPR_P,     259);
        SYMDEF(L4_EXPR,       260);
        SYMDEF(L3_EXPR_P,     261);
        SYMDEF(L3_EXPR,       262);
        SYMDEF(L2_EXPR_P,     263);
        SYMDEF(L2_EXPR,       264);
        SYMDEF(L1_EXPR_P,     265);
        SYMDEF(L1_EXPR,       266);
        SYMDEF(EXPR,          267);
        SYMDEF(SYM_OR_FCAL,   268);
        SYMDEF(ARGS_E,        269);
        SYMDEF(COMMA_SEP_E,   270);
        SYMDEF(COMMA_SEP,     271);
        SYMDEF(COMMA_SEP_P,   272);
        SYMDEF(VARDEF,        273);
        SYMDEF(STMT,          274);
        SYMDEF(STMTS,         275);
        SYMDEF(IFSTMT,        276);
        SYMDEF(ELSECLAUSE,    277);
        SYMDEF(WHILESTMT,     278);
        SYMDEF(FNDEF,         279);
        SYMDEF(VIRTVARDEFS,   280);
        SYMDEF(VIRTVARDEFS_P, 281);
        SYMDEF(PROGRAM_PART,  282);
        SYMDEF(PROGRAM,       283);
        SYMDEF(UNARY_OP,      284);
        SYMDEF(BI_OP4,        285);
        SYMDEF(BI_OP3,        286);
        SYMDEF(BI_OP2,        287);
        SYMDEF(BI_OP1,        288);
        SYMDEF(TYPE,          289);

#undef SYMDEF

    edited = true;
    return smap;
}

std::string pl0cc::symbols::symbolToName(Symbol s) {
    auto& smap = symbols::symbolToNameMap();
    if (smap.count(s)) {
        return smap.at(s);
    }
    return tokenTypeName(static_cast<TokenType>(s));
}

// If exception throws token count
SyntaxTree pl0cc::llZeroParseSyntax(const Syntax &syntax, const TokenStorage &ts) {
    std::shared_ptr<SyntaxTree> stt = std::make_shared<SyntaxTree>(syntax.start());
    
    std::stack<std::shared_ptr<SyntaxTree>> symbolStack;
    symbolStack.push(stt);


    auto llMap = syntax.llMap();

    auto tokenIter = ts.begin();
    int lineCounter = 0;
    int tokenCounter = 0;
    while (!symbolStack.empty()) {
        auto sp = symbolStack.top();
        symbolStack.pop();
        while (tokenIter->type == TokenType::NEWLINE) {
            tokenIter++;
            lineCounter++;
            tokenCounter = 0;
        }

        if (!syntax.nonTerminatingSymbols().count(sp->symbol())) {
            if (tokenIter == ts.end() || Symbol(tokenIter->type) != sp->symbol()) {
                throw std::tuple<int, int, int>(tokenIter - ts.begin(), lineCounter, tokenCounter);
            }
            sp->setTokenData(*tokenIter++);
            tokenCounter++;
            continue;
        }

        auto sent = llMap[sp->symbol()][Symbol(tokenIter->type)];
        sp->setChildSentence(sent);
        for (size_t i = sp->childCount() - 1; i < sp->childCount(); i--) {
            symbolStack.push(sp->shareChild(i));
        }
    }

    return *stt;
}