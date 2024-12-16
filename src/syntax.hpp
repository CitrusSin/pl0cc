#ifndef PL0CC_SYNTAX_HPP
#define PL0CC_SYNTAX_HPP

#include <algorithm>
#include <functional>
#include <initializer_list>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <unordered_set>

#include "lexer.hpp"


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

        Symbol start() const;
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

    class SyntaxTree {
    public:
        explicit SyntaxTree(Token token);
        explicit SyntaxTree(Symbol symbol);
        
        Symbol symbol() const;
        void addChild(SyntaxTree st);
        size_t childCount() const;
        bool childExists(size_t index) const;
        const SyntaxTree& childAt(size_t index) const;
        SyntaxTree& childAt(size_t index);
        std::shared_ptr<SyntaxTree> shareChild(size_t index);
        void setChildSentence(const Sentence& sentence);
        void setTokenData(Token token);

        void serializeTo(std::ostream& os, std::function<std::string(Symbol)> symbolName, int tabCount = 0);
    private:
        Symbol symbolData;
        std::optional<Token> tokenData;
        std::vector<std::shared_ptr<SyntaxTree>> childs;
    };

    namespace symbols {
#define SYMDEF(name, value) const Symbol name = (value)

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

        const std::map<Symbol, std::string>& symbolToNameMap();
        std::string symbolToName(Symbol s);
    }

    Syntax genSyntax();

    SyntaxTree llZeroParseSyntax(const Syntax& syntax, const TokenStorage& ts);
}

#endif
