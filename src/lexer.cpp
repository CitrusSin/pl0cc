#include "lexer.hpp"
#include "nondeterministic_automaton.hpp"
#include "regex.hpp"

#include <string>
#include <string_view>
#include <sstream>
#include <cstring>
#include <utility>

namespace pl0cc {
    constexpr static const std::array TOKEN_REGEXS {
            /*COMMENT*/ "//|/\\*",
                        "if",
                        "else",
                        "for",
                        "while",
                        "break",
                        "return",
                        "continue",
                        "float",
                        "int",
                        "char",
            /*SYMBOL*/  "[_a-zA-Z][_a-zA-Z0-9]*",
            /*NUMBER*/  "0|[1-9][0-9]*|(0|[1-9][0-9]*)?.[0-9]+([eE][-+]?[0-9]+)?",
                        "\\+",
                        "-",
                        "\\*",
                        "/",
                        "%",
                        ">",
                        ">=",
                        "<",
                        "<=",
                        "!=",
                        "==",
                        "!",
                        "&&",
                        "\\|\\|",
                        ",",
                        "=",
                        "\\[",
                        "\\]",
                        "\\(",
                        "\\)",
                        "\\{",
                        "\\}",
                        ";",
                        "\\.",
            /*NEWLINE*/ "\r|\n|\r\n", // Support different newline for different platforms
            /*EOF*/     "",
            /*CMTSTOP*/ "\\*/"
    };

    static std::unique_ptr<const DeterministicAutomaton> automaton = nullptr;

    void Lexer::buildAutomaton() {
        if (automaton != nullptr) return;

        NondeterministicAutomaton nfa;
        auto start = nfa.startSingleState();
        nfa.addJump(start, ' ', nfa.startSingleState());
        nfa.addStateMarkup(start, 0);   // Mark 0 to start state for feedChar()
        for (int type = 0; type < TOKEN_REGEXS.size(); type++) {
            if (strlen(TOKEN_REGEXS[type]) == 0) {
                continue;
            }
            auto subAtm = automatonFromRegexString(TOKEN_REGEXS[type]);
            subAtm.addEndStateMarkup(type);
            nfa.addAutomaton(start, subAtm);
        }

        automaton = std::make_unique<DeterministicAutomaton>(nfa.toDeterministic());
    }

    Lexer::Lexer() :
        tokenQueue(),
        lineCounter(0), columnCounter(0),
        hasStopped(false),
        storedLines(1, ""), errors()
    {
        if (automaton == nullptr) buildAutomaton();
        state = automaton->startState();
    }

    bool Lexer::feedChar(char ch) {
        using State = DeterministicAutomaton::State;
        using TokenType = Token::TokenType;

        State trialState = automaton->nextState(state, ch);

        // When rejected, a new token shall be generated or there's an error happening
        if (trialState == DeterministicAutomaton::REJECT) {
            // Check if it is an error
            const std::set<int> &marks = automaton->stateMarkup(state);
            if (!automaton->isStopState(state) || marks.empty()) {
                int colStart = columnCounter - (int)readingToken.size();
                if (colStart < 0) colStart = 0;
                errors.emplace_back(lineCounter, colStart, readingToken.size() + 1);
                readingToken.clear();
                trialState = automaton->startState();
            } else {
                TokenType type = TokenType(*marks.begin()); // Take the smallest mark (see token type class id as priority)

                if (type == Token::NEWLINE) {
                    lineCounter++;
                    columnCounter = 0;
                    storedLines.emplace_back();
                    if (lastCommentToken == "//") {
                        // If single line comment, exit comment mode
                        lastCommentToken = "";
                    }
                } else if (marks.count(Token::CMTSTOP) && !lastCommentToken.empty() && lastCommentToken == "/*") {
                    lastCommentToken = "";
                } else if (type == Token::COMMENT) {
                    lastCommentToken = readingToken;
                }

                if (lastCommentToken.empty() && type != Token::CMTSTOP && type != Token::COMMENT) {
                    // Do not really add token when comment mode is on
                    tokenQueue.emplace_back(type, std::move(readingToken));
                }
                readingToken = std::string("");
                trialState = automaton->nextState(automaton->startState(), ch);
            }
        } else if (!lastCommentToken.empty() && lastCommentToken == "/*" && automaton->stateMarkup(state).count(Token::CMTSTOP)) {
            // Stop comment mode
            lastCommentToken = "";
            trialState = automaton->startState();
        }

        state = trialState;
        columnCounter++;
        storedLines.back().push_back(ch);

        if (trialState != automaton->startState()) readingToken.push_back(ch);
        return false;
    }

    void Lexer::feedStream(std::istream &stream) {
        int c;
        while (c = stream.get(), stream) {
            feedChar(static_cast<char>(c));
        }
        eof();
    }

    bool Lexer::tokenEmpty() const {
        return tokenQueue.empty();
    }

    size_t Lexer::tokenCount() const {
        return tokenQueue.size();
    }

    Token Lexer::takeToken() {
        Token val = std::move(tokenQueue.front());
        tokenQueue.pop_front();
        return val;
    }

    void Lexer::eof() {
        using TokenType = Token::TokenType;

        if (!lastCommentToken.empty()) {
            tokenQueue.emplace_back(Token::TOKEN_EOF);
            hasStopped = true;
            return;
        }

        const std::set<int> &marks = automaton->stateMarkup(state);
        if (!automaton->isStopState(state) || marks.empty()) {
            errors.emplace_back(lineCounter, columnCounter, readingToken.size());
        } else {
            TokenType type = TokenType(*marks.begin()); // Take the smallest mark (see token type class id as priority)

            if (type == Token::NEWLINE) {
                lineCounter++;
                columnCounter = 0;
                storedLines.emplace_back();
            }

            tokenQueue.emplace_back(type, std::move(readingToken));
            readingToken = std::string("");
            state = automaton->startState();
        }
        tokenQueue.emplace_back(Token::TOKEN_EOF);
        hasStopped = true;
    }

    bool Lexer::stopped() const {
        return hasStopped;
    }

    size_t Lexer::errorCount() const {
        return errors.size();
    }

    Lexer::ErrorReport Lexer::errorReportAt(size_t idx) const {
        return errors[idx];
    }

    const std::string &Lexer::sourceLine(int lineNumber) const {
        return storedLines[lineNumber];
    }

    Token::Token(Token::TokenType type, std::string content)
        : _type(type), _content(std::move(content)) {}

    Token::TokenType Token::type() const {
        return _type;
    }

    const std::string &Token::content() const {
        return _content;
    }

    std::string Token::serialize() const {
        constexpr static const std::array typeMap {
            "COMMENT", "IF", "ELSE", "FOR", "WHILE",
            "BREAK", "RETURN", "CONTINUE", "FLOAT", "INT",
            "CHAR", "SYMBOL", "NUMBER", "OP_PLUS", "OP_SUB",
            "OP_MUL", "OP_DIV", "OP_MOD", "OP_GT", "OP_GE",
            "OP_LT", "OP_LE", "OP_NEQ", "OP_EQU", "OP_NOT",
            "OP_AND", "OP_OR", "OP_COMMA", "OP_ASSIGN", "LMBRACKET",
            "RMBRACKET", "LSBRACKET", "RSBRACKET", "LLBRACKET", "RLBRACKET",
            "SEMICOLON", "DOT", "NEWLINE", "TOKEN_EOF", "CMTSTOP"
        };
        std::stringstream ss;
        ss << "TokenType: " << int(_type) << " (" << typeMap[_type] << ")";
        size_t len = ss.str().size();
        while (len < 30) {
            ss << ' ';
            len++;
        }
        ss << "Content: " << _content;
        return ss.str();
    }
}