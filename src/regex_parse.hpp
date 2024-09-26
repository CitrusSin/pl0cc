#ifndef PL0CC_REGEX_PARSE_HPP
#define PL0CC_REGEX_PARSE_HPP

#include <string>
#include <deque>
#include <memory>

#include "nondeterministic_automaton.hpp"

namespace pl0cc {
    class RegexToken {
    public:
        virtual ~RegexToken() = default;

        enum type {
            STRING, CHAR_SELECTOR, OPERATOR, LEFT_BRACKET, RIGHT_BRACKET
        };
        [[nodiscard]] virtual type getType() const = 0;
        [[nodiscard]] virtual std::string serialize() const = 0;
    };

    class PlainString : public RegexToken {
    public:
        explicit PlainString(std::string_view content) : _content(content) {}

        [[nodiscard]]
        constexpr const std::string& content() const {
            return _content;
        }

        [[nodiscard]] type getType() const override {
            return STRING;
        }

        [[nodiscard]] std::string serialize() const override {
            return "PLAIN_STRING\"" + _content + "\"";
        }
    private:
        std::string _content;
    };

    class Operator : public RegexToken {
    public:
        ~Operator() override = default;

        [[nodiscard]] virtual int priority() const = 0;
        [[nodiscard]] virtual int operandCount() const = 0;
        [[nodiscard]] virtual char content() const = 0;
        virtual void applyOperator(std::deque<NondeterministicAutomaton>& operands) = 0;

        virtual std::string serialize() const override {
            return std::string("OPERATOR\'") + content() + "\'";
        }

        virtual type getType() const override {
            return OPERATOR;
        }

        static constexpr inline bool isOperator(char c);
        static std::unique_ptr<Operator> operatorFrom(char op);
    };

    class OperatorBracket : public Operator {
    public:
        explicit OperatorBracket(char ch = '(') : _content(ch) {}

        [[nodiscard]] int priority() const override       { return -1; }
        [[nodiscard]] int operandCount() const override  { return (_content == ')') ? 1 : 2; }
        [[nodiscard]] char content() const override       { return _content; }

        [[nodiscard]] type getType() const override { return (_content == '(') ? LEFT_BRACKET : RIGHT_BRACKET; }

        [[nodiscard]] std::string serialize() const override { return (_content == '(') ? "LEFT_BRACKET" : "RIGHT_BRACKET"; }

        void applyOperator(std::deque<NondeterministicAutomaton>& operands) override {}
    private:
        char _content;
    };

    class OperatorPlus : public Operator {
    public:
        [[nodiscard]] int priority() const override       { return 2; }
        [[nodiscard]] int operandCount() const override  { return 1; }
        [[nodiscard]] char content() const override       { return '+'; }

        void applyOperator(std::deque<NondeterministicAutomaton>& operands) override {
            operands.back().refactorToRepetitive();
        }
    };

    class OperatorOptional : public Operator {
    public:
        [[nodiscard]] int priority() const override       { return 2; }
        [[nodiscard]] int operandCount() const override  { return 1; }
        [[nodiscard]] char content() const override       { return '?'; }

        void applyOperator(std::deque<NondeterministicAutomaton>& operands) override {
            operands.back().refactorToSkippable();
        }
    };

    class OperatorAsterisk : public Operator {
    public:
        [[nodiscard]] int priority() const override       { return 2; }
        [[nodiscard]] int operandCount() const override  { return 1; }
        [[nodiscard]] char content() const override       { return '*'; }

        void applyOperator(std::deque<NondeterministicAutomaton>& operands) override {
            operands.back().refactorToRepetitive();
            operands.back().refactorToSkippable();
        }
    };

    class OperatorConcatenate : public Operator {
    public:
        [[nodiscard]] int priority() const override       { return 1; }
        [[nodiscard]] int operandCount() const override  { return 2; }
        [[nodiscard]] char content() const override       { return 'C'; }

        [[nodiscard]] std::string serialize() const override { return "CONNECT"; }

        void applyOperator(std::deque<NondeterministicAutomaton>& operands) override {
            NondeterministicAutomaton& r2 = operands[operands.size() - 1];
            NondeterministicAutomaton& r1 = operands[operands.size() - 2];
            r1.connect(r2);
            operands.pop_back();
        }
    };

    class OperatorOr : public Operator {
    public:
        [[nodiscard]] int priority() const override       { return 0; }
        [[nodiscard]] int operandCount() const override  { return 2; }
        [[nodiscard]] char content() const override       { return '|'; }

        void applyOperator(std::deque<NondeterministicAutomaton>& operands) override {
            NondeterministicAutomaton& r2 = operands[operands.size() - 1];
            NondeterministicAutomaton& r1 = operands[operands.size() - 2];
            r1.makeOriginBranch(r2);
            operands.pop_back();
        }
    };

    class CharSelector : public RegexToken {
    public:
        explicit CharSelector(std::string_view sv) : _content(sv.substr(1, sv.size() - 2)) {}

        [[nodiscard]] type getType() const override { return CHAR_SELECTOR; }
        [[nodiscard]] std::string serialize() const override { return "SELECTOR[" + _content + "]"; }

        [[nodiscard]] std::string content() const { return _content; }
    private:
        std::string _content;
    };

    constexpr inline bool Operator::isOperator(char c) {
        switch (c) {
        case '(':
        case ')':
        case '+':
        case '?':
        case '*':
        case '|':
        case '\0':
            return true;
        default:
            return false;
        }
    }

    inline std::unique_ptr<Operator> Operator::operatorFrom(char op) {
        switch (op) {
        case '(':
            return std::make_unique<OperatorBracket>('(');
        case ')':
            return std::make_unique<OperatorBracket>(')');
        case '+':
            return std::make_unique<OperatorPlus>();
        case '?':
            return std::make_unique<OperatorOptional>();
        case '*':
            return std::make_unique<OperatorAsterisk>();
        case '|':
            return std::make_unique<OperatorOr>();
        case '\0':
            return std::make_unique<OperatorConcatenate>();
        default:
            return std::make_unique<OperatorConcatenate>();
        }
    }

    std::vector<std::shared_ptr<RegexToken>> regexTokenize(std::string_view sv);
    NondeterministicAutomaton buildNfa(const std::vector<std::shared_ptr<RegexToken>>& tokens);
    NondeterministicAutomaton stringAutomaton(std::string_view s);
    NondeterministicAutomaton selectorAutomaton(const CharSelector& selector);
} // pl0cc

#endif // PL0CC_REGEX_PARSE_HPP