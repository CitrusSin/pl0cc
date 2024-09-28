#ifndef PL0CC_DETERMINISTIC_AUTOMATON_HPP
#define PL0CC_DETERMINISTIC_AUTOMATON_HPP

#include <string>
#include <limits>
#include <set>
#include <utility>
#include <vector>
#include <map>

namespace pl0cc {
    class DeterministicAutomaton {
    public:
        using State = size_t;
        using EncodeUnit = unsigned char;
        constexpr static const State REJECT = std::numeric_limits<size_t>::max();

        DeterministicAutomaton();

        [[nodiscard]] inline size_t stateCount() const { return stateMap.size(); }

        State addState();
        [[nodiscard]] State startState() const;
        void setJump(State from, EncodeUnit ch, State to);
        [[nodiscard]] State nextState(State from, EncodeUnit ch) const;
        void setStopState(State s, bool stop = true);
        [[nodiscard]] bool isStopState(State s) const;

        void addStateMarkup(State s, int mark);
        void removeStateMarkup(State s, int mark);
        void removeStateMarkup(State s);
        [[nodiscard]] const std::set<int>& stateMarkup(State s) const;

        std::pair<State, std::set<State>> importAutomaton(const DeterministicAutomaton& atm);

        void simplify();

        [[nodiscard]] std::string serialize() const;
    private:
        std::vector<std::map<EncodeUnit, State>> stateMap;
        std::vector<std::set<int>> stateMarks;
        State _startState;
        std::set<State> _endStates;
    };
}

#endif