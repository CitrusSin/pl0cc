//
// Created by Citrus on 2024/9/25.
//

#ifndef PL0CC_LEXER_HPP
#define PL0CC_LEXER_HPP

#include <iostream>
#include <memory>
#include <utility>

#include "deterministic_automaton.hpp"

namespace pl0cc {
    enum class TokenType: unsigned int {
        COMMENT   = 0,  FN        = 1,  IF        = 2,  ELSE      = 3,  FOR       = 4,
        WHILE     = 5,
        BREAK     = 6,  RETURN    = 7,  CONTINUE  = 8,  FLOAT     = 9,  INT       = 10,
        CHAR      = 11, SYMBOL    = 12, NUMBER    = 13, OP_PLUS   = 14, OP_SUB    = 15,
        OP_MUL    = 16, OP_DIV    = 17, OP_MOD    = 18, OP_GT     = 19, OP_GE     = 20,
        OP_LT     = 21, OP_LE     = 22, OP_NEQ    = 23, OP_EQU    = 24, OP_NOT    = 25,
        OP_AND    = 26, OP_OR     = 27, COMMA     = 28, ASSIGN    = 29, LMBRACKET = 30,
        RMBRACKET = 31, LSBRACKET = 32, RSBRACKET = 33, LLBRACKET = 34, RLBRACKET = 35,
        SEMICOLON = 36, DOT       = 37, NEWLINE   = 38, TOKEN_EOF = 39, STRING    = 40,
        ARROW     = 41
    };

    std::string tokenTypeName(TokenType type);

    class RawToken {
    public:

        explicit RawToken(TokenType type, std::string content = "")
            : _type(type), _content(std::move(content)) {}

        [[nodiscard]] TokenType type() const {return _type;}
        [[nodiscard]] const std::string& content() const {return _content;}
        [[nodiscard]] std::string& content() {return _content;}

        [[nodiscard]] std::string serialize() const;
    private:
        TokenType _type;
        std::string _content;
    };

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

    class Lexer {
    public:
        enum class ErrorType {
            INVALID_CHAR, READING_TOKEN, NONSTOP_TOKEN
        };

        class ErrorReport {
        public:
            ErrorReport(Lexer *lexer, ErrorType type, int lineCounter, int colCounter, int tokenLength, std::set<int> readingTokenType) :
                    lexer(lexer), type(type), lineNum(lineCounter), colNum(colCounter), tokenLen(tokenLength), readingTokenType(std::move(readingTokenType)) {}
            [[nodiscard]] constexpr ErrorType errorType() const {return type;}
            [[nodiscard]] constexpr int lineNumber() const {return lineNum;}
            [[nodiscard]] constexpr int columnNumber() const {return colNum;}
            [[nodiscard]] constexpr int tokenLength() const {return tokenLen;}
            [[nodiscard]] std::set<int> tokenTypes() const {return readingTokenType;}
            void reportErrorTo(std::ostream &output, bool colorful = true);
        private:
            Lexer *lexer;
            ErrorType type;
            int lineNum, colNum, tokenLen;
            std::set<int> readingTokenType;
        };

        Lexer();

        TokenStorage& tokenStorage();

        // true if new token generated
        bool feedChar(char ch);
        void feedStream(std::istream& stream);
        void eof();

        [[nodiscard]] bool tokenEmpty() const;
        [[nodiscard]] size_t tokenCount() const;

        [[nodiscard]] bool stopped() const;

        [[nodiscard]] size_t errorCount() const;
        [[nodiscard]] ErrorReport errorReportAt(size_t index) const;
        [[nodiscard]] const std::string& sourceLine(int lineNumber) const;

        static const DeterministicAutomaton& getDFA();
    private:
        /*
        enum class CommentState {
            NONE, SINGLE_LINE, MULTI_LINE
        };
        */

        DeterministicAutomaton::State state;
        TokenStorage storage;
        int lineCounter, columnCounter;
        bool hasStopped;
        std::string readingToken;
        std::vector<std::string> storedLines;
        std::vector<ErrorReport> errors;
        //std::string lastCommentToken;
        //CommentState commentState;

        static std::unique_ptr<const DeterministicAutomaton> automaton;

        bool generateTokenAndReset();
        void pushError(ErrorType type, const std::set<int>& possibleTokenTypes = {});

        template<typename... Args>
        void pushToken(Args &&... args) {
            storage.pushToken(RawToken(std::forward<Args>(args)...));
        }

        static void buildAutomaton();
    };

} // pl0cc

#endif // PL0CC_LEXER_HPP
