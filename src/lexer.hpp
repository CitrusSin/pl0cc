//
// Created by Citrus on 2024/9/25.
//

#ifndef PL0CC_LEXER_HPP
#define PL0CC_LEXER_HPP

#include <iostream>
#include <deque>
#include <memory>
#include <utility>

#include "deterministic_automaton.hpp"

namespace pl0cc {
    class Token {
    public:
        enum TokenType {
            COMMENT   = 0,  IF        = 1,  ELSE      = 2,  FOR       = 3,  WHILE     = 4,
            BREAK     = 5,  RETURN    = 6,  CONTINUE  = 7,  FLOAT     = 8,  INT       = 9,
            CHAR      = 10, SYMBOL    = 11, NUMBER    = 12, OP_PLUS   = 13, OP_SUB    = 14,
            OP_MUL    = 15, OP_DIV    = 16, OP_MOD    = 17, OP_GT     = 18, OP_GE     = 19,
            OP_LT     = 20, OP_LE     = 21, OP_NEQ    = 22, OP_EQU    = 23, OP_NOT    = 24,
            OP_AND    = 25, OP_OR     = 26, OP_COMMA  = 27, OP_ASSIGN = 28, LMBRACKET = 29,
            RMBRACKET = 30, LSBRACKET = 31, RSBRACKET = 32, LLBRACKET = 33, RLBRACKET = 34,
            SEMICOLON = 35, DOT       = 36, NEWLINE   = 37, TOKEN_EOF = 38, CMTSTOP   = 39
        };

        explicit Token(TokenType type, std::string content = "");

        [[nodiscard]] TokenType type() const;
        [[nodiscard]] const std::string& content() const;

        [[nodiscard]] std::string serialize() const;

        static std::string serializeTokenType(TokenType type);
    private:
        TokenType _type;
        std::string _content;
    };

    class Lexer {
    public:
        class ErrorReport {
        public:
            ErrorReport(Lexer *lexer, int lineCounter, int colCounter, int tokenLength, std::set<int> readingTokenType) :
                    lexer(lexer), lineNum(lineCounter), colNum(colCounter), tokenLen(tokenLength), readingTokenType(std::move(readingTokenType)) {}
            [[nodiscard]] constexpr int lineNumber() const {return lineNum;}
            [[nodiscard]] constexpr int columnNumber() const {return colNum;}
            [[nodiscard]] constexpr int tokenLength() const {return tokenLen;}
            [[nodiscard]] std::set<int> tokenTypes() const {return readingTokenType;}
            void reportErrorTo(std::ostream &output);
        private:
            Lexer *lexer;
            int lineNum, colNum, tokenLen;
            std::set<int> readingTokenType;
        };

        Lexer();

        // true if new token generated
        bool feedChar(char ch);
        void feedStream(std::istream& stream);
        void eof();

        [[nodiscard]] bool tokenEmpty() const;
        [[nodiscard]] size_t tokenCount() const;
        Token takeToken();

        [[nodiscard]] bool stopped() const;

        [[nodiscard]] size_t errorCount() const;
        [[nodiscard]] ErrorReport errorReportAt(size_t index) const;
        [[nodiscard]] const std::string& sourceLine(int lineNumber) const;

    private:
        DeterministicAutomaton::State state;
        std::deque<Token> tokenQueue;
        int lineCounter, columnCounter;
        bool hasStopped;
        std::string readingToken;
        std::vector<std::string> storedLines;
        std::vector<ErrorReport> errors;
        std::string lastCommentToken;

        static void buildAutomaton();
    };

}

#endif
