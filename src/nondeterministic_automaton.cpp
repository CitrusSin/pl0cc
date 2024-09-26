#include <sstream>
#include <stack>
#include <algorithm>
#include "nondeterministic_automaton.hpp"

using namespace pl0cc;

using EncodeUnit = NondeterministicAutomaton::EncodeUnit;

NondeterministicAutomaton::NondeterministicAutomaton() : nodes(1), startSstate(0) {}

NondeterministicAutomaton::State NondeterministicAutomaton::State::nextState(EncodeUnit next) const {
    return atm->nextState(*this, next);
}

NondeterministicAutomaton::State& NondeterministicAutomaton::State::next(EncodeUnit next) {
    return *this = atm->nextState(*this, next);
}

NondeterministicAutomaton::State& NondeterministicAutomaton::State::operator+=(const State& s2) {
    insert(s2.begin(), s2.end());
    return *this;
}

std::set<EncodeUnit> NondeterministicAutomaton::State::characterTransitions() const {
    return atm->characterTransitions(*this);
}

std::set<int> NondeterministicAutomaton::State::stateMarkups() const {
    std::set<int> marks;
    for (SingleState ss : *this) {
        const std::set<int>& sms = atm->stateMarkups(ss);
        marks.insert(sms.begin(), sms.end());
    }
    return marks;
}

NondeterministicAutomaton::SingleState NondeterministicAutomaton::addState() {
    nodes.emplace_back();
    return nodes.size() - 1;
}

void NondeterministicAutomaton::addJump(SingleState from, EncodeUnit ch, SingleState to) {
    nodes[from].next.insert(std::make_pair(ch, to));
}

void NondeterministicAutomaton::addEpsilonJump(SingleState from, SingleState to) {
    nodes[from].epsNext.insert(to);
}

bool NondeterministicAutomaton::containsEpsilonJump(SingleState from, SingleState to) const {
    return nodes[from].epsNext.count(to) > 0;
}

NondeterministicAutomaton::State NondeterministicAutomaton::epsilonClosure(SingleState s) const {
    return epsilonClosure(stateOf({s}));
}

NondeterministicAutomaton::State NondeterministicAutomaton::epsilonClosure(State states) const {
    std::stack<SingleState> searchStack;
    for (SingleState s : states) {
        searchStack.push(s);
    }

    while (!searchStack.empty()) {
        SingleState st = searchStack.top();
        searchStack.pop();

        for (SingleState next : nodes[st].epsNext) {
            if (!states.count(next)) {
                states.insert(next);
                searchStack.push(next);
            }
        }
    }

    return states;
}

NondeterministicAutomaton::State NondeterministicAutomaton::nextState(SingleState prev, EncodeUnit ch) const {
    auto iteratorBegin = nodes[prev].next.find(ch);
    auto iteratorEnd = iteratorBegin;

    State st = stateOf({});
    while (iteratorEnd != nodes[prev].next.end() && iteratorEnd->first == ch) {
        st.insert(iteratorEnd->second);
        iteratorEnd++;
    }

    return epsilonClosure(st);
}

NondeterministicAutomaton::State NondeterministicAutomaton::nextState(const State& prev, EncodeUnit ch) const {
    State s = stateOf({});
    for (SingleState ss : prev) {
        auto it = nodes[ss].next.find(ch);
        while (it != nodes[ss].next.end() && it->first == ch) {
            s.insert((it++)->second);
        }
    }
    return epsilonClosure(s);
}

std::set<EncodeUnit> NondeterministicAutomaton::characterTransitions(SingleState sstate) const {
    std::set<EncodeUnit> transitions;

    for (auto& [ch, next] : nodes[sstate].next) {
        transitions.insert(ch);
    }

    return transitions;
}

std::set<EncodeUnit> NondeterministicAutomaton::characterTransitions(const State& state) const {
    std::set<EncodeUnit> transitions;

    for (SingleState sstate : state) {
        for (auto& [ch, next] : nodes[sstate].next) {
            transitions.insert(ch);
        }
    }

    return transitions;
}

NondeterministicAutomaton::State NondeterministicAutomaton::startState() const {
    return epsilonClosure(startSstate);
}

NondeterministicAutomaton::SingleState NondeterministicAutomaton::startSingleState() const {
    return startSstate;
}

void NondeterministicAutomaton::setStopState(SingleState s, bool stop) {
    if (stop) {
        stopSstates.insert(s);
    } else {
        stopSstates.erase(s);
    }
}

bool NondeterministicAutomaton::isStopState(SingleState s) const {
    return stopSstates.count(s);
}

bool NondeterministicAutomaton::isStopState(const State& s) const {
    for (auto ss: s) {
        if (isStopState(ss)) return true;
    }

    return false;
}

void NondeterministicAutomaton::addStateMarkup(SingleState s, int mark) {
    nodes[s].marks.insert(mark);
}

void NondeterministicAutomaton::removeStateMarkup(SingleState s, int mark) {
    nodes[s].marks.erase(mark);
}

void NondeterministicAutomaton::setStateMarkups(SingleState s, const std::set<int>& marks) {
    nodes[s].marks = marks;
}

const std::set<int>& NondeterministicAutomaton::stateMarkups(SingleState s) const {
    return nodes[s].marks;
}

void NondeterministicAutomaton::addEndStateMarkup(int mark) {
    for (SingleState ss : stopSstates) {
        addStateMarkup(ss, mark);
    }
}

void NondeterministicAutomaton::addAutomaton(SingleState from, const NondeterministicAutomaton& atm) {
    auto [start, stop] = importAutomaton(atm);

    addEpsilonJump(from, start);
    stopSstates.insert(stop.begin(), stop.end());
}

void NondeterministicAutomaton::refactorToRepetitive() {
    unifyStopSingleStates();

    if (stopSstates.empty()) {
        return;
    }

    if (containsEpsilonJump(*stopSstates.begin(), startSstate)) {
        return;
    }

    addEpsilonJump(*stopSstates.begin(), startSstate);
}

void NondeterministicAutomaton::refactorToSkippable() {
    unifyStopSingleStates();

    if (stopSstates.empty()) {
        return;
    }

    if (containsEpsilonJump(startSstate, *stopSstates.begin())) {
        return;
    }

    addEpsilonJump(startSstate, *stopSstates.begin());
}

void NondeterministicAutomaton::connect(const NondeterministicAutomaton& atm) {
    unifyStopSingleStates();

    SingleState sstate = *stopSstates.begin();
    stopSstates.clear();

    addAutomaton(sstate, atm);
}

void NondeterministicAutomaton::makeOriginBranch(const NondeterministicAutomaton& m2) {
    addAutomaton(startSstate, m2);
}

template <typename T>
static std::string serializeSet(const std::set<T>& val) {
    if (val.size() == 0) {
        return "{}";
    }

    std::stringstream serializeStream;
    if (val.size() == 1) {
        serializeStream << *val.begin();
        return serializeStream.str();
    }

    serializeStream << '{';

    bool mark = false;
    for (auto v : val) {
        if (mark) serializeStream << ',';
        serializeStream << v;
        mark = true;
    }

    serializeStream << '}';

    return serializeStream.str();
}

DeterministicAutomaton NondeterministicAutomaton::toDeterministic() const {
    const NondeterministicAutomaton &nfa = *this;

    DeterministicAutomaton atm;

    NondeterministicAutomaton::State nfaState = nfa.startState();

    std::map<NondeterministicAutomaton::State, DeterministicAutomaton::State> stateTranslate;
    stateTranslate[nfaState] = atm.startState();

    std::deque<NondeterministicAutomaton::State> stateQueue;
    stateQueue.push_back(nfaState);

    while (!stateQueue.empty()) {
        NondeterministicAutomaton::State& st = stateQueue.front();
        DeterministicAutomaton::State fst = stateTranslate[st];

        for (EncodeUnit ch : st.characterTransitions()) {
            NondeterministicAutomaton::State nextState = st.nextState(ch);
            DeterministicAutomaton::State nextDetState;
            if (!stateTranslate.count(nextState)) {
                nextDetState = stateTranslate[nextState] = atm.addState();
                atm.setStopState(nextDetState, nfa.isStopState(nextState));
                stateQueue.push_back(nextState);
            } else {
                nextDetState = stateTranslate[nextState];
            }
            atm.setJump(fst, ch, nextDetState);
        }

        stateQueue.pop_front();
    }

    // Pass State markups marked by other programs
    for (auto& [nfa_state, dfa_state] : stateTranslate) {
        for (int mark : nfa_state.stateMarkups()) {
            atm.addStateMarkup(dfa_state, mark);
        }
    }

    atm.simplify();

    return atm;
}

std::string NondeterministicAutomaton::serialize() const {
    std::stringstream serializeStream;
    for (SingleState ss = 0; ss < stateCount(); ss++) {
        serializeStream << "STATE" << ss << ": {";

        bool mark1 = false;
        if (!nodes[ss].epsNext.empty()) {
            serializeStream << "EPS -> " << serializeSet(nodes[ss].epsNext);
            mark1 = true;
        }

        auto& nextMap = nodes[ss].next;

        EncodeUnit lastChar = '\0';
        std::set<SingleState> lastSet;
        for (auto it = nextMap.begin(); it != nextMap.end(); it++) {
            if (lastChar == '\0') lastChar = it->first;
            if (lastChar != it->first) {
                if (mark1) serializeStream << ',';
                mark1 = true;
                serializeStream << char(lastChar) << " -> " << serializeSet(lastSet);
                lastSet.clear();
                lastChar = it->first;
            }
            lastSet.insert(it->second);
        }
        if (!lastSet.empty()) {
            if (mark1) serializeStream << ',';
            mark1 = true;
            serializeStream << char(lastChar) << " -> " << serializeSet(lastSet);
            lastSet.clear();
        }

        serializeStream << "}\n";
    }

    serializeStream << "FINISH_STATES = " << serializeSet(stopSstates) << "\n";
    return serializeStream.str();
}

// PRIVATE FUNCTIONS
std::pair<NondeterministicAutomaton::SingleState, std::set<NondeterministicAutomaton::SingleState>>
NondeterministicAutomaton::importAutomaton(const NondeterministicAutomaton& atm) {
    SingleState bias = nodes.size();
    for (SingleState src = 0; src < atm.nodes.size(); src++) {
        state_node next_node;
        for (auto [ch, st] : atm.nodes[src].next) {
            next_node.next.emplace(ch, st + bias);
        }
        for (auto st : atm.nodes[src].epsNext) {
            next_node.epsNext.emplace(st + bias);
        }
        // Marks remain unchanged
        next_node.marks = atm.nodes[src].marks;

        nodes.push_back(std::move(next_node));
    }

    SingleState start_sstate = atm.startSstate + bias;
    std::set<SingleState> stop_sstates;
    for (auto s : atm.stopSstates) {
        stop_sstates.insert(s + bias);
    }

    return make_pair(start_sstate, std::move(stop_sstates));
}

NondeterministicAutomaton::State NondeterministicAutomaton::stateOf(std::initializer_list<SingleState> sstates) const {
    return State(this, sstates);
}

void NondeterministicAutomaton::unifyStopSingleStates() {
    if (stopSstates.size() <= 1) return;

    SingleState new_stop = addState();
    for (SingleState sstate : stopSstates) {
        addEpsilonJump(sstate, new_stop);
    }

    stopSstates = {new_stop};
}
