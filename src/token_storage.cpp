#include <sstream>
#include "lexer.hpp"

namespace pl0cc {
    TokenStorage::TokenStorage() = default;

    void TokenStorage::pushToken(RawToken token) {
        int seman;
        TokenType type = token.type();

        switch (type) {
            case TokenType::SYMBOL:
                if (!symbolMap.count(token.content())) {
                    seman = symbolMap[token.content()] = static_cast<int>(symbols.size());
                    symbols.push_back(std::move(token.content()));
                } else {
                    seman = symbolMap[token.content()];
                }
                break;
            case TokenType::NUMBER:
                if (!numberConstantMap.count(token.content())) {
                    seman = numberConstantMap[token.content()] = static_cast<int>(numberConstants.size());
                    numberConstants.push_back(std::move(token.content()));
                } else {
                    seman = numberConstantMap[token.content()];
                }
                break;
            default:
                seman = -1;
                break;
        }

        tokens.emplace_back(type, seman);
    }

    void TokenStorage::serializeTo(std::ostream& ss) const {
        ss << "Tokens >--------------------\n";
        ss << "Type            Seman\n";
        for (auto token : tokens) {
            std::stringstream ssLine;

            ssLine << int(token.type);
            while (ssLine.tellp() < 2) ssLine << ' ';

            ssLine << "(" << tokenTypeName(token.type) << ")";
            while (ssLine.tellp() < 16) ssLine << ' ';

            if (token.seman == -1) {
                ssLine << '^';
            } else {
                ssLine << token.seman;
            }
            ss << ssLine.str() << '\n';
        }
        ss << '\n';

        ss << "Symbols >-------------------\n";
        ss << "Index  Value\n";
        for (size_t i=0; i<symbols.size(); i++) {
            std::stringstream ssLine;

            ssLine << i;
            while (ssLine.tellp() < 7) ssLine << ' ';

            ssLine << symbols[i];

            ss << ssLine.str()<< '\n';
        }
        ss << '\n';

        ss << "Numbers >-------------------\n";
        ss << "Index  Value\n";
        for (size_t i=0; i<numberConstants.size(); i++) {
            std::stringstream ssLine;

            ssLine << i;
            while (ssLine.tellp() < 7) ssLine << ' ';

            ssLine << numberConstants[i];

            ss << ssLine.str()<< '\n';
        }
        ss << '\n';
    }
} // pl0cc