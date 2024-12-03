#ifndef PL0CC_GRAMMAR_HPP
#define PL0CC_GRAMMAR_HPP

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <limits>
#include <map>
#include <set>
#include <string_view>
#include <utility>
#include <vector>
#include <unordered_set>


namespace pl0cc {
    using Symbol = unsigned int;
    constexpr const Symbol EPS = std::numeric_limits<unsigned int>::max();

    class Sentence {
    public:
        using Iterator = std::vector<Symbol>::iterator;
        using ConstIterator = std::vector<Symbol>::const_iterator;

        Sentence() : sequence() {}

        template <typename Iter>
        Sentence(Iter begin, Iter end) : sequence(begin, end) {}

        Sentence(std::initializer_list<Symbol> init_list) : sequence(init_list.begin(), init_list.end()) {}

        template <typename T>
        Sentence(std::basic_string_view<T> sv) : sequence(sv.begin(), sv.end()) {}

        Sentence(const char* str) : Sentence(std::string_view(str)) {}

        Sentence(std::vector<Symbol> sequence) : sequence(std::move(sequence)) {}

        size_t size() const {return sequence.size();}

        Symbol& operator[](size_t idx) {return sequence[idx];}
        const Symbol& operator[](size_t idx) const {return sequence[idx];}

        Iterator begin() {return sequence.begin();}
        ConstIterator begin() const {return sequence.begin();}
        Iterator end() {return sequence.end();}
        ConstIterator end() const {return sequence.end();}

        void push_back(const Symbol& s) {
            sequence.push_back(s);
        }

        Sentence substr(size_t start, size_t len = std::numeric_limits<size_t>::max()) const {
            len = std::min(len, size() - start);
            return Sentence(begin() + start, begin() + start + len);
        }

        Sentence operator+(const Sentence& sv2) const {
            Sentence s = *this;
            for (Symbol v : sv2) s.push_back(v);
            return s;
        }

        bool operator==(const Sentence& s2) const {
            if (this == &s2) return true;
            return sequence == s2.sequence;
        }
    private:
        std::vector<Symbol> sequence;
    };
}

namespace std {
    template <>
    struct hash<pl0cc::Sentence> {
        size_t operator()(const pl0cc::Sentence& sentence) const noexcept {
            size_t ans = 0;
            for (const auto& v : sentence) {
                ans *= 257; // 257 is the smallest prime greater than 2^8=256
                ans += v;
            }
            return ans;
        }
    };
}

namespace pl0cc {
    class Syntax {
    public:
        Syntax(Symbol startSymbol) : emptySymbolSetValid(false), firstSetsValid(false), followSetsValid(false), startSymbol(startSymbol) {
            addSymbol(startSymbol);
        }

        void addConduct(Symbol leftPart, Sentence rightPart);

        const std::set<Symbol>& symbols() const;
        const std::set<Symbol>& nonTerminatingSymbols() const;
        const std::vector<std::pair<Symbol, Sentence>>& conducts() const;
        
        const std::set<Symbol>& firstSet(Symbol s) const;
		std::set<Symbol> firstSet(const Sentence& stmt) const;

        const std::set<Symbol>& followSet(Symbol s) const;

        std::set<Symbol> selectSet(Symbol leftPart, const Sentence& rightPart) const;

        std::map<Symbol, std::map<Symbol, Sentence>> llMap() const;
    private:
        Symbol startSymbol;
        std::set<Symbol> symbolSet, ntSymbolSet;
        std::map<Symbol, std::unordered_set<Sentence>> sentences;
        std::vector<std::pair<Symbol, Sentence>> conductVector;

        mutable bool emptySymbolSetValid;
        mutable std::set<Symbol> emptySymbolSet;
        mutable bool firstSetsValid;
        mutable std::map<Symbol, std::set<Symbol>> firstSets;
        mutable bool followSetsValid;
        mutable std::map<Symbol, std::set<Symbol>> followSets;

        Symbol addSymbol(Symbol sym);

        void searchEmptySymbols() const;
        void calculateFirstSet() const;
        std::set<Symbol> firstSetFor(Symbol s, std::set<Symbol>& searchPath, std::map<Symbol, std::set<Symbol>>& storage) const;
        void calculateFollowSet() const;
    };
}

#endif
