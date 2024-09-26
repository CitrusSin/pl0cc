#include "lexer.hpp"
#include "nondeterministic_automaton.hpp"
#include "regex.hpp"

#include <string>
#include <string_view>
#include <sstream>
#include <cstring>
#include <utility>

using namespace std::literals;

namespace pl0cc {
    constexpr static const char* tokenRegexs[] {
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
    constexpr static const char* typeMap[] {
            "COMMENT", "IF", "ELSE", "FOR", "WHILE",
            "BREAK", "RETURN", "CONTINUE", "FLOAT", "INT",
            "CHAR", "SYMBOL", "NUMBER", "OP_PLUS", "OP_SUB",
            "OP_MUL", "OP_DIV", "OP_MOD", "OP_GT", "OP_GE",
            "OP_LT", "OP_LE", "OP_NEQ", "OP_EQU", "OP_NOT",
            "OP_AND", "OP_OR", "OP_COMMA", "OP_ASSIGN", "LMBRACKET",
            "RMBRACKET", "LSBRACKET", "RSBRACKET", "LLBRACKET", "RLBRACKET",
            "SEMICOLON", "DOT", "NEWLINE", "TOKEN_EOF", "CMTSTOP"
    };

    static std::unique_ptr<const DeterministicAutomaton> automaton = nullptr;

    void Lexer::buildAutomaton() {
        using SingleState = NondeterministicAutomaton::SingleState;

        if (automaton != nullptr) return;

        NondeterministicAutomaton nfa;
        auto start = nfa.startSingleState();
        nfa.addJump(start, ' ', nfa.startSingleState());
        nfa.addJump(start, '\t', nfa.startSingleState());
        nfa.addStateMarkup(start, 0);   // Mark 0 to start state for feedChar()
        constexpr const int regexLen = sizeof tokenRegexs / sizeof tokenRegexs[0];
        for (int type = 0; type < regexLen; type++) {
            if (/*strlen(tokenRegexs[type]) == 0*/ tokenRegexs[type][0] == '\0') {
                continue;
            }
            auto subAtm = automatonFromRegexString(tokenRegexs[type]);
            /*
             * Mark end nodes with 2*type+1 and mark non-end nodes with 2*type,
             * which will be splited by splitMarkup() below
             */
            subAtm.addEndStateMarkup((type << 1) | 1);
            for (SingleState subState = 0; subState < subAtm.stateCount(); subState++) {
                if (!subAtm.isStopState(subState)) subAtm.addStateMarkup(subState, type << 1);
            }
            nfa.addAutomaton(start, subAtm);
        }

        auto dfa = std::make_unique<DeterministicAutomaton>(nfa.toDeterministic());
        dfa->removeAllStateMarkup(dfa->startState());
        automaton = std::move(dfa);
    }

    Lexer::Lexer() :
        tokenQueue(),
        lineCounter(0), columnCounter(0),
        hasStopped(false),
        storage(nullptr),
        storedLines(1, ""), errors()
    {
        if (automaton == nullptr) buildAutomaton();
        state = automaton->startState();
    }

    static std::pair<std::set<int>, std::set<int>> splitMarkup(const std::set<int>& markups) {
        std::set<int> p0, p1;
        for (int m : markups) {
            if (m & 1) {
                p1.insert(m >> 1);
            } else {
                p0.insert(m >> 1);
            }
        }
        return std::make_pair(p0, p1);
    }

    bool Lexer::feedChar(char ch) {
        using State = DeterministicAutomaton::State;

        State trialState = automaton->nextState(state, ch);

        bool tokenGenerated = false;
        // When rejected, a new token shall be generated or there's an error happening
        if (trialState == DeterministicAutomaton::REJECT) {
            // Check if it is an error
            auto [procedureMarks, stopMarks] = splitMarkup(automaton->stateMarkup(state));
            if (!automaton->isStopState(state) || stopMarks.empty()) {
                int colStart = columnCounter - (int)readingToken.size();
                if (colStart < 0) colStart = 0;
                if (lastCommentToken.empty()) {
                    errors.emplace_back(this, lineCounter, colStart, readingToken.size() + 1, procedureMarks);
                }
                readingToken.clear();
                trialState = automaton->nextState(automaton->startState(), ch);
                if (trialState == DeterministicAutomaton::REJECT) {
                    trialState = automaton->startState();
                }
            } else {
                TokenType type = TokenType(*stopMarks.begin()); // Take the smallest mark (see token type class id as priority)

                if (type == TokenType::NEWLINE) {
                    lineCounter++;
                    columnCounter = 0;
                    // Remove redundant newlines
                    while (storedLines.back().back() == '\r' || storedLines.back().back() == '\n') {
                        storedLines.back().pop_back();
                    }
                    storedLines.emplace_back();
                    if (lastCommentToken == "//") {
                        // If comment mode is in single line comment, exit comment mode
                        lastCommentToken = "";
                    }
                } else if (stopMarks.count((int)TokenType::CMTSTOP) && !lastCommentToken.empty() && lastCommentToken == "/*") {
                    lastCommentToken = "";
                } else if (type == TokenType::COMMENT) {
                    lastCommentToken = readingToken;
                }

                if (
                        // Do not really add token when comment mode is on
                        lastCommentToken.empty() && type != TokenType::CMTSTOP && type != TokenType::COMMENT
                        // Even if comment mode is on, newline still works to make sure line number is correct
                        || type == TokenType::NEWLINE
                ) {
                    tokenQueue.emplace_back(type, std::move(readingToken));
                    tokenGenerated = true;
                }
                readingToken = std::string("");
                trialState = automaton->nextState(automaton->startState(), ch);
                if (trialState == DeterministicAutomaton::REJECT) {
                    // Report error
                    int colStart = columnCounter - (int)readingToken.size();
                    if (colStart < 0) colStart = 0;
                    if (lastCommentToken.empty()) {
                        errors.emplace_back(this, lineCounter, colStart, readingToken.size() + 1, std::set<int>{});
                    }
                    readingToken.clear();
                    trialState = automaton->startState();
                }
            }
        } else if (lastCommentToken == "/*" && automaton->stateMarkup(state).count((int)TokenType::CMTSTOP)) {
            // Even the automaton has not stopped, comment mode should be stopped when CMTSTOP appears
            // Stop comment mode
            lastCommentToken = "";
            trialState = automaton->startState();
            readingToken.clear();
        }

        state = trialState;
        columnCounter++;
        storedLines.back().push_back(ch);

        if (trialState != automaton->startState()) readingToken.push_back(ch);

        if (storage) {
            while (!tokenEmpty()) {
                this->storage->pushToken(takeToken());
            }
        }

        return tokenGenerated;
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

    RawToken Lexer::takeToken() {
        RawToken val = std::move(tokenQueue.front());
        tokenQueue.pop_front();
        return val;
    }

    void Lexer::eof() {
        if (!lastCommentToken.empty()) {
            tokenQueue.emplace_back(TokenType::TOKEN_EOF);
            hasStopped = true;
            return;
        }

        auto [procedureMarks, stopMarks] = splitMarkup(automaton->stateMarkup(state));
        if (!automaton->isStopState(state) || stopMarks.empty()) {
            errors.emplace_back(this, lineCounter, columnCounter, readingToken.size(), procedureMarks);
        } else {
            TokenType type = TokenType(*stopMarks.begin()); // Take the smallest mark (see token type class id as priority)

            if (type == TokenType::NEWLINE) {
                lineCounter++;
                columnCounter = 0;
                storedLines.emplace_back();
            }

            tokenQueue.emplace_back(type, std::move(readingToken));
            readingToken = std::string("");
            state = automaton->startState();
        }
        tokenQueue.emplace_back(TokenType::TOKEN_EOF);
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

    void Lexer::setTokenStorage(TokenStorage *storage) {
        this->storage = storage;
        while (!tokenEmpty()) {
            this->storage->pushToken(takeToken());
        }
    }

    std::string RawToken::serialize() const {
        std::stringstream ss;
        ss << "TokenType: " << int(_type) << " (" << typeMap[int(_type)] << ")";

        if (_type == TokenType::NEWLINE) {
            return ss.str();
        }

        size_t len = ss.str().size();
        while (len < 30) {
            ss << ' ';
            len++;
        }
        ss << "Content: " << _content;
        return ss.str();
    }

    std::string tokenTypeName(TokenType type) {
        return typeMap[static_cast<int>(type)];
    }

    void Lexer::ErrorReport::reportErrorTo(std::ostream &output, bool colorful) {
        const char* CONSOLE_RED = "\033[31m";
        const char* CONSOLE_RESET = "\033[0m";

        if (!colorful) {
            CONSOLE_RED = CONSOLE_RESET = "";
        }

        const std::string& srcLine = lexer->sourceLine(lineNumber());
        std::stringstream hintLine;
        bool needReset = false;
        for (int idx = 0; idx < srcLine.size(); idx++) {
            if (idx == columnNumber()) {
                hintLine << CONSOLE_RED;
                needReset = true;
            }
            if (idx == columnNumber() + tokenLength()) {
                hintLine << CONSOLE_RESET;
                needReset = false;
            }
            hintLine << srcLine[idx];
        }
        if (needReset) hintLine << CONSOLE_RESET;

        std::string reason;
        if (tokenTypes().empty()) {
            reason = "Read unknown character '"s
                     + srcLine[columnNumber() + tokenLength() - 1]
                     + "'";
        } else {
            std::stringstream ss;
            ss << "Read invalid character '" << srcLine[columnNumber() + tokenLength() - 1] << "' ";

            ss << "while reading possible token { ";
            for (auto tokenType : tokenTypes()) {
                ss << tokenTypeName(TokenType(tokenType)) << ' ';
            }
            ss << "}";
            reason = ss.str();
        }

        output << "---------------------" << std::endl;
        output << lineNumber()+1 << " |\t" << hintLine.str() << std::endl;
        output << "Reason: " << reason << std::endl << std::endl;
    }
}