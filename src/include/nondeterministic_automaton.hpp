#ifndef PL0CC_NONDETERMINISTIC_AUTOMATON_HPP
#define PL0CC_NONDETERMINISTIC_AUTOMATON_HPP

#include <initializer_list>
#include <vector>
#include <string>
#include <map>
#include <set>

#include "deterministic_automaton.hpp"


namespace pl0cc {
    class NondeterministicAutomaton {
    public:
        using SingleState = size_t;
        using EncodeUnit = unsigned char;
        
        class State : private std::set<SingleState> {
        public:
            [[nodiscard]] State nextState(EncodeUnit next) const;
            State& next(EncodeUnit next);
            State& operator+=(const State& s2);

            // Expose compare functions
#define _EXT_CMP(cmp) \
            inline bool operator cmp (const State& s2) const {    \
                if (atm == s2.atm) { \
                    return  static_cast<const std::set<SingleState>&>(*this)   \
                            cmp static_cast<const std::set<SingleState>&>(s2); \
                } else { \
                    return atm cmp s2.atm;\
                } \
            }

            _EXT_CMP(<)  _EXT_CMP(==)  _EXT_CMP(>)
            _EXT_CMP(<=) _EXT_CMP(!=)  _EXT_CMP(>=)
#undef _EXT_CMP

            [[nodiscard]] std::set<EncodeUnit> characterTransitions() const;
            [[nodiscard]] std::set<int> stateMarkups() const;

        private:
            friend class NondeterministicAutomaton;

            const NondeterministicAutomaton* atm;

            explicit State(const NondeterministicAutomaton* atm) : std::set<SingleState>(), atm(atm) {}
            State(const NondeterministicAutomaton* atm, std::initializer_list<SingleState> il) : std::set<SingleState>(std::move(il)), atm(atm) {}
        };

        NondeterministicAutomaton();

        [[nodiscard]] inline size_t stateCount() const { return nodes.size(); }

        SingleState addState();
        void addJump(SingleState from, EncodeUnit ch, SingleState to);
        void addEpsilonJump(SingleState from, SingleState to);
        [[nodiscard]] bool containsEpsilonJump(SingleState from, SingleState to) const;
        [[nodiscard]] State epsilonClosure(SingleState s) const;
        [[nodiscard]] State epsilonClosure(State states) const;
        [[nodiscard]] State nextState(SingleState prev, EncodeUnit ch) const;
        [[nodiscard]] State nextState(const State& prev, EncodeUnit ch) const;
        [[nodiscard]] std::set<EncodeUnit> characterTransitions(SingleState sstate) const;
        [[nodiscard]] std::set<EncodeUnit> characterTransitions(const State& state) const;
        [[nodiscard]] State startState() const;
        [[nodiscard]] SingleState startSingleState() const;
        void setStopState(SingleState s, bool stop = true);
        [[nodiscard]] bool isStopState(SingleState s) const;
        [[nodiscard]] bool isStopState(const State& s) const;

        void addStateMarkup(SingleState s, int mark);
        void removeStateMarkup(SingleState s, int mark);
        void setStateMarkups(SingleState s, const std::set<int>& marks);
        [[nodiscard]] const std::set<int>& stateMarkups(SingleState s) const;
        void addEndStateMarkup(int mark);

        void addAutomaton(SingleState from, const NondeterministicAutomaton& atm);
        void refactorToRepetitive();
        void refactorToSkippable();
        void connect(const NondeterministicAutomaton& atm);
        void makeOriginBranch(const NondeterministicAutomaton& m2);

        [[nodiscard]] std::string serialize() const;

        [[nodiscard]] DeterministicAutomaton toDeterministic() const;
    private:
        struct state_node {
            std::multimap<EncodeUnit, SingleState> next;
            std::set<SingleState> epsNext;
            std::set<int> marks;
        };

        std::vector<state_node> nodes;
        SingleState startSstate;
        std::set<SingleState> stopSstates;

        std::pair<SingleState, std::set<SingleState>> importAutomaton(const NondeterministicAutomaton& atm);
        [[nodiscard]] State stateOf(std::initializer_list<SingleState> sstates) const;
        void unifyStopSingleStates();
    };
}


#endif