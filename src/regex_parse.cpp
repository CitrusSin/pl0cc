#include "regex_parse.hpp"
#include <array>
#include <memory>
#include <stdexcept>

#define assert(condition) \
    if (!(condition)) throw std::runtime_error("Assertion failed: "#condition)

namespace pl0cc {
    std::vector<std::shared_ptr<RegexToken>> regexTokenize(std::string_view sv) {
        std::vector<std::shared_ptr<RegexToken>> tokens;

        bool isReadingString = true;

        std::string_view::size_type fromIndex, toIndex;
        fromIndex = toIndex = 0;
        for (toIndex = 0; toIndex < sv.size(); toIndex++) {
            if (sv[toIndex] == '\\' && toIndex + 1 < sv.size()) {
                // Escape
                toIndex++;
                continue;
            }
            if (Operator::isOperator(sv[toIndex])) {
                if (fromIndex != toIndex) {
                    tokens.push_back(std::make_shared<PlainString>(sv.substr(fromIndex, toIndex - fromIndex)));
                    if (sv[toIndex] == '(') {
                        tokens.push_back(std::make_shared<OperatorConcatenate>());
                    }
                } else if (sv[toIndex] == '(' && !isReadingString) {
                    tokens.push_back(std::make_shared<OperatorConcatenate>());
                }
                tokens.push_back(Operator::operatorFrom(sv[toIndex]));
                fromIndex = toIndex + 1;
                isReadingString = Operator::operatorFrom(sv[toIndex])->operandCount() == 2;
                continue;
            }
            if (
                    fromIndex < toIndex &&
                    toIndex + 1 < sv.size() &&
                    Operator::isOperator(sv[toIndex + 1]) &&
                    Operator::operatorFrom(sv[toIndex + 1])->priority() > OperatorConcatenate().priority()
            ) {
                tokens.push_back(std::make_shared<PlainString>(sv.substr(fromIndex, toIndex - fromIndex)));
                tokens.push_back(std::make_shared<OperatorConcatenate>());
                tokens.push_back(std::make_shared<PlainString>(sv.substr(toIndex, 1)));
                fromIndex = toIndex + 1;
                isReadingString = true;
                continue;
            }
            if (!isReadingString) {
                tokens.push_back(std::make_shared<OperatorConcatenate>());
            }
            isReadingString = true;
            if (sv[toIndex] == '[') {
                if (fromIndex != toIndex) {
                    tokens.push_back(std::make_shared<PlainString>(sv.substr(fromIndex, toIndex - fromIndex)));
                    tokens.push_back(std::make_shared<OperatorConcatenate>());
                }
                std::string_view::size_type left, right;
                left = right = toIndex;
                while (sv[right] != ']') {
                    if (sv[right] == '\\') right++;
                    right++;
                }
                tokens.push_back(std::make_shared<CharSelector>(sv.substr(left, right - left + 1)));
                fromIndex = toIndex = right;
                fromIndex++;
                isReadingString = false;
                continue;
            }
        }

        if (fromIndex != toIndex) {
            tokens.push_back(std::make_shared<PlainString>(sv.substr(fromIndex, toIndex - fromIndex)));
        }

        return tokens;
    }

    NondeterministicAutomaton buildNfa(const std::vector<std::shared_ptr<RegexToken>>& tokens) {
        std::deque<NondeterministicAutomaton> operands;
        std::deque<std::shared_ptr<RegexToken>> opers;

        for (const std::shared_ptr<RegexToken>& tk : tokens) {
            switch (tk->getType()) {
            case RegexToken::STRING:
                operands.push_back(stringAutomaton(dynamic_cast<PlainString &>(*tk).content()));
                break;
            case RegexToken::CHAR_SELECTOR:
                operands.push_back(selectorAutomaton(dynamic_cast<CharSelector &>(*tk)));
                break;
            case RegexToken::OPERATOR:
                {
                    auto& op = dynamic_cast<Operator&>(*tk);
                    while (
                            !opers.empty() &&
                                    opers.back()->getType() == RegexToken::OPERATOR &&
                            dynamic_cast<Operator&>(*opers.back()).priority() > op.priority()
                    ) {
                        dynamic_cast<Operator &>(*opers.back()).applyOperator(operands);
                        opers.pop_back();
                    }
                    opers.push_back(std::dynamic_pointer_cast<Operator>(tk));
                }
                break;
            case RegexToken::LEFT_BRACKET:
                opers.push_back(std::dynamic_pointer_cast<Operator>(tk));
                break;
            case RegexToken::RIGHT_BRACKET:
                {
                    while (
                            !opers.empty() &&
                                    opers.back()->getType() == RegexToken::OPERATOR
                    ) {
                        dynamic_cast<Operator &>(*opers.back()).applyOperator(operands);
                        opers.pop_back();
                    }
                    if (!opers.empty() && opers.back()->getType() == RegexToken::LEFT_BRACKET) {
                        opers.pop_back();
                    }
                }
                break;
            }
        }

        while (!opers.empty()) {
            assert(opers.back()->getType() == RegexToken::OPERATOR);
            dynamic_cast<Operator &>(*opers.back()).applyOperator(operands);
            opers.pop_back();
        }

        assert(operands.size() == 1);

        return operands.back();
    }

    NondeterministicAutomaton stringAutomaton(std::string_view s) {
        NondeterministicAutomaton atm;

        auto state = atm.startSingleState();
        for (size_t i = 0; i < s.size(); i++) {
            char c = s[i];
            if (c == '\\' && i + 1 < s.size()) {
                c = s[++i]; // Escape once
            }
            auto nextState = atm.addState();
            atm.addJump(state, c, nextState);
            state = nextState;
        }
        atm.setStopState(state);

        return atm;
    }

    NondeterministicAutomaton selectorAutomaton(const CharSelector& selector) {
        using SingleState = NondeterministicAutomaton::SingleState;
        
        std::string selContent = selector.content();

        std::array<bool, std::numeric_limits<unsigned char>::max()+1> charSel{};
        charSel.fill(false);
        bool negative = false;
        for (size_t i = 0; i < selContent.size(); i++) {
            if (i == 0 && selContent[i] == '^') {
                negative = true;
                continue;
            }
            if (selContent[i] == '\\' && i + 1 < selContent.size()) {
                // Escape once
                i++;
                if (selContent[i] == '-') {
                    charSel['-'] = true;
                    continue;
                }
            }
            if (i+2 < selContent.size() && selContent[i + 1] == '-') {
                char from = selContent[i], to = selContent[i + 2];
                if (to == '\\' && i+3 < selContent.size()) {
                    to = selContent[i + 3];
                    i = i+3;
                } else {
                    i = i+2;
                }
                for (char c = from; c <= to; c++) {
                    charSel[c] = true;
                }
                continue;
            }
            charSel[selContent[i]] = true;
        }
        if (negative) {
            for (bool& val : charSel) {
                val = !val;
            }
        }


        NondeterministicAutomaton atm;
        SingleState startState, stopState;

        startState = atm.startSingleState();
        atm.setStopState(stopState = atm.addState());
        
        for (unsigned int ch=0x0; ch<=std::numeric_limits<unsigned char>::max(); ch++) {
            if (charSel[ch]) {
                atm.addJump(startState, ch, stopState);
            }
        }

        return atm;
    }
}