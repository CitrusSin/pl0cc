#ifndef PL0CC_TOKEN_STORAGE_HPP
#define PL0CC_TOKEN_STORAGE_HPP

#include <vector>
#include "lexer.hpp"

namespace pl0cc {
    struct Token {
        TokenType type;
        int seman;

        Token(TokenType type, int seman) : type(type), seman(seman) {}
    };

    class TokenStorage {
    public:
        TokenStorage();

        void pushToken(RawToken token);

        void serializeTo(std::ostream& ss) const;

        [[nodiscard]] size_t size() const { return tokens.size(); }
        Token operator[](size_t idx) const { return tokens[idx]; }

        [[nodiscard]] auto begin() -> std::vector<Token>::iterator {return tokens.begin();}
        [[nodiscard]] auto begin() const -> std::vector<Token>::const_iterator {return tokens.begin();}
        [[nodiscard]] auto end() -> std::vector<Token>::iterator {return tokens.end();}
        [[nodiscard]] auto end() const -> std::vector<Token>::const_iterator {return tokens.end();}
    private:
        std::vector<Token> tokens;

        std::vector<std::string> symbols, numberConstants, stringConstants;
        std::map<std::string, int> symbolMap, numberConstantMap, stringConstantMap;
    };
} // pl0cc

#endif //PL0CC_TOKEN_STORAGE_HPP
