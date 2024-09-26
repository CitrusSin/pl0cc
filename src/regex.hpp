#ifndef PL0CC_REGEX_HPP
#define PL0CC_REGEX_HPP

#include <memory>
#include <string_view>
#include <vector>

#include "deterministic_automaton.hpp"
#include "nondeterministic_automaton.hpp"
#include "regex_parse.hpp"

namespace pl0cc {
    class Regex {
    public:
        explicit Regex(std::string_view sv);

        bool match(std::string_view sv) const;
        std::vector<std::string> tokens() const;
        NondeterministicAutomaton& automaton();
        const NondeterministicAutomaton& automaton() const;
        const DeterministicAutomaton& deterministicAutomaton() const;
    private:
        std::vector<std::shared_ptr<RegexToken>> _tokens;
        NondeterministicAutomaton _atm;
        mutable std::unique_ptr<DeterministicAutomaton> _dfaPtr;

        void makeDfa() const;
    };

    namespace literal {
        Regex operator"" _regex(const char* str, size_t len);
    }

    using namespace literal;

    NondeterministicAutomaton automatonFromRegexString(std::string_view str);
} // pl0cc

#endif // PL0CC_REGEX_HPP