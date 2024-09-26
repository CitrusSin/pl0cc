
#include "deterministic_automaton.hpp"
#include <map>
#include <sstream>
#include <utility>
#include <vector>
#include <functional>

using namespace pl0cc;
using State = DeterministicAutomaton::State;

DeterministicAutomaton::DeterministicAutomaton() : stateMap(1), stateMarks(1), _startState(0), _endStates() {}

State DeterministicAutomaton::addState() {
    stateMap.emplace_back();
    stateMarks.emplace_back();
    return stateMap.size() - 1;
}

State DeterministicAutomaton::startState() const {
    return _startState;
}

void DeterministicAutomaton::setJump(State from, EncodeUnit ch, State to) {
    stateMap[from][ch] = to;
}

State DeterministicAutomaton::nextState(State from, EncodeUnit ch) const {
    if (from == REJECT) return REJECT;
    if (stateMap[from].count(ch)) return stateMap[from].at(ch);
    return REJECT;
}

void DeterministicAutomaton::setStopState(State s, bool stop) {
    if (stop) {
        _endStates.insert(s);
    } else {
        _endStates.erase(s);
    }
}

bool DeterministicAutomaton::isStopState(State s) const {
    return _endStates.count(s) > 0;
}

void DeterministicAutomaton::addStateMarkup(State s, int mark) {
    stateMarks[s].insert(mark);
}

void DeterministicAutomaton::removeStateMarkup(State s, int mark) {
    stateMarks[s].erase(mark);
}

const std::set<int>& DeterministicAutomaton::stateMarkup(State s) const {
    return stateMarks[s];
}

std::pair<State, std::set<State>> DeterministicAutomaton::importAutomaton(const DeterministicAutomaton& atm) {
    size_t bias = stateCount();
    stateMap.insert(stateMap.end(), atm.stateMap.begin(), atm.stateMap.end());
    stateMarks.insert(stateMarks.end(), atm.stateMarks.begin(), atm.stateMarks.end());

    for (size_t i = bias; i < stateMap.size(); i++) {
        for (auto& [ch, target] : stateMap[i]) {
            target += bias;
        }
    }

    State start = atm._startState + bias;
    std::set<State> stopStates;
    for (State s : atm._endStates) {
        stopStates.insert(s + bias);
    }
    return std::make_pair(start, stopStates);
}

void DeterministicAutomaton::simplify() {
    class Dsu {
    public:
        explicit Dsu(size_t n) : values(n, 0) {}

        void reset(size_t x) {values[x] = x;}
        size_t root(size_t x) {
            if (values[x] == x) return x;
            return values[x] = root(values[x]);
        }
        void link(size_t a, size_t b) {values[root(a)] = root(b);}
    private:
        std::vector<size_t> values;
    };

    Dsu equivalence(stateCount());
    std::map<std::set<int>, State> markStates;

    for (State s = 0; s < stateCount(); s++) {
        if (isStopState(s)) {
            equivalence.reset(s);
            std::set<int> mark = stateMarkup(s);
            if (markStates.count(mark)) {
                equivalence.link(s, markStates[mark]);
            } else {
                markStates[stateMarkup(s)] = s;
            }
        }
    }

    using MapComparator = std::function<bool(const std::map<EncodeUnit, State>&, const std::map<EncodeUnit, State>&)>;
    const MapComparator skipTableEquals = [&](const std::map<EncodeUnit, State>& table1, const std::map<EncodeUnit, State>& table2) {
        if (table1.size() != table2.size()) return false;

        for (auto [k, v] : table1) {
            if (!table2.count(k)) return false;
            if (equivalence.root(v) != equivalence.root(table2.at(k))) {
                return false;
            }
        }
        
        return true;
    };

    bool hasChanges;
    do {
        hasChanges = false;
        for (State s0 = 0; s0 < stateCount(); s0++) {
            State s1 = equivalence.root(s0);
            if (s0 != s1 && !skipTableEquals(stateMap[s0], stateMap[s1])) {
                equivalence.reset(s0);
                hasChanges = true;
            }
        }
    } while (hasChanges);

    std::vector<State> stateMappings(stateCount(), REJECT);
    for (State s = 0, sc = 0; s < stateCount(); s++) {
        if (equivalence.root(s) == s) {
            stateMappings[s] = sc++;
        }
    }

    std::vector<State> removeStates;
    for (State s = 0; s < stateCount(); s++) {
        if (equivalence.root(s) != s) {
            removeStates.push_back(s);
            continue;
        }

        auto& sMap = stateMap[s];
        for (auto& [ch, st] : sMap) {
            sMap[ch] = stateMappings[equivalence.root(st)];
        }
    }

    int bias = 0;
    for (State s : removeStates) {
        stateMap.erase(stateMap.begin() + s - bias);
        stateMarks.erase(stateMarks.begin() + s - bias);
        bias++;
    }

    std::set<State> newStopStates;
    for (State s : _endStates) {
        newStopStates.insert(stateMappings[equivalence.root(s)]);
    }
    _endStates = newStopStates;
    _startState = stateMappings[equivalence.root(_startState)];
}

std::string DeterministicAutomaton::serialize() const {
    std::stringstream serializeStream;
    for (State s = 0; s < stateCount(); s++) {
        serializeStream << "STATE" << s << ": {";
        bool mark = false;
        for (auto [ch, st] : stateMap[s]) {
            if (mark) serializeStream << ", ";
            serializeStream << char(ch) << " -> " << st;
            mark = true;
        }
        serializeStream << "}\n";
    }
    serializeStream << "STOP_STATES =";
    for (State s : _endStates) {
        serializeStream << ' ' << s;
    }
    serializeStream << '\n';

    return serializeStream.str();
}

void DeterministicAutomaton::removeAllStateMarkup(State s) {
    stateMarks[s].clear();
}

