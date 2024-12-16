#include <cstdlib>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "lexer.hpp"
#include "syntax.hpp"

using namespace std;
using pl0cc::Lexer, pl0cc::TokenStorage;

const char* CONSOLE_RED = "\033[31m";
const char* CONSOLE_GREEN = "\033[32m";
const char* CONSOLE_RESET = "\033[0m";

int main(int argc, char **argv) {
    std::string inputFilename, outputFilename;
    bool showAutomaton = false;
    int rd = 1;
    while (rd < argc) {
        std::string_view s(argv[rd]);
        if (s == "-o" && rd + 1 < argc) {
            outputFilename = argv[++rd];
        } else if (s == "--automaton") {
            showAutomaton = true;
        } else {
            inputFilename = argv[rd];
        }
        rd++;
    }

    if (inputFilename.empty()) {
        clog << "pl0cc: " << CONSOLE_RED << "Error" << CONSOLE_RESET << ": Input file not specified." << endl;
        return EXIT_FAILURE;
    }

    if (outputFilename.empty()) {
        clog << "pl0cc: " << CONSOLE_RED << "Error" << CONSOLE_RESET << ": Output file not specified." << endl;
        return EXIT_FAILURE;
    }

    clog << "pl0cc v0.1\n";

    if (showAutomaton) {
        clog << "Automaton >--------------\n";
        clog << Lexer::getDFA().serialize() << '\n';
    }

    auto absoluteInputPath = filesystem::absolute(inputFilename);

    ifstream input(inputFilename);
    Lexer lexer;
    TokenStorage& ts = lexer.tokenStorage();

    lexer.feedStream(input);
    input.close();

    if (!lexer.stopped()) {
        clog << "pl0cc: " << CONSOLE_RED << "Error" << CONSOLE_RESET << ": Lexer hasn't stopped." << endl;
        return EXIT_FAILURE;
    }

    clog << "pl0cc completed with ";
    if (lexer.errorCount() == 0) {
        pl0cc::Syntax st = pl0cc::genSyntax();
        std::optional<pl0cc::SyntaxTree> optTree;
        try {
            optTree = pl0cc::llZeroParseSyntax(st, ts);
        } catch (std::tuple<int, int, int> err) {
            clog << "Syntax parser reported an " << CONSOLE_RED << "error" << CONSOLE_RESET << " at line " << (std::get<1>(err)+1) << " token " << (std::get<2>(err)+1) << "." << endl;
            clog << "---------------------" << std::endl;
            clog << std::get<1>(err)+1 << " |\t" << lexer.sourceLine(std::get<1>(err)) << std::endl;
            return EXIT_FAILURE;
        }

        clog << CONSOLE_GREEN << "0" << CONSOLE_RESET << " errors occurred." << endl;

        ofstream output(outputFilename);
        ts.serializeTo(output);
        optTree.value().serializeTo(output, pl0cc::symbols::symbolToName);

        output << flush;
        output.close();
    } else {
        clog << CONSOLE_RED << lexer.errorCount() << CONSOLE_RESET << " lexer errors occurred." << endl;
        clog << endl;
        for (size_t i=0; i<lexer.errorCount(); i++) {
            Lexer::ErrorReport report = lexer.errorReportAt(i);

            string srcFile = absoluteInputPath.string() + ":" + to_string(report.lineNumber() + 1) + ":" + to_string(report.columnNumber() + 1);
            clog << "Error " << (i+1) << " at " << srcFile << ": " << std::endl;
            report.reportErrorTo(clog);
        }
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}