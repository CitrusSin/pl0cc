
#include "regex.hpp"
#include <memory>
#include <string>

namespace pl0cc {
    Regex::Regex(std::string_view sv) :
            _tokens(regexTokenize(sv)),
            _atm(buildNfa(_tokens)),
            _dfaPtr(nullptr)
    {}

    bool Regex::match(std::string_view sv) const {
        makeDfa();

        DeterministicAutomaton::State s = _dfaPtr->startState();
        for (char c : sv) {
            s = _dfaPtr->nextState(s, c);
        }
        
        return _dfaPtr->isStopState(s);
    }

    std::vector<std::string> Regex::tokens() const {
        std::vector<std::string> tks(_tokens.size());
        for (size_t i=0; i < _tokens.size(); i++) {
            tks[i] = _tokens[i]->serialize();
        }
        return tks;
    }

    NondeterministicAutomaton& Regex::automaton() {
        return _atm;
    }

    const NondeterministicAutomaton& Regex::automaton() const {
        return _atm;
    }

    const DeterministicAutomaton& Regex::deterministicAutomaton() const {
        makeDfa();
        return *_dfaPtr;
    }

    void Regex::makeDfa() const {
        if (_dfaPtr == nullptr) {
            _dfaPtr = std::make_unique<DeterministicAutomaton>(_atm.toDeterministic());
        }
    }

    Regex literal::operator"" _regex(const char* str, size_t len) {
        return Regex(std::string_view(str, len));
    }

    NondeterministicAutomaton automatonFromRegexString(std::string_view str) {
        return buildNfa(regexTokenize(str));
    }
}