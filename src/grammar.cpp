#include "grammar.hpp"
#include <cmath>
#include <queue>
#include <set>
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
