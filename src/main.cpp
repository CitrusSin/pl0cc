#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

#include "lexer.hpp"

using namespace std;
using pl0cc::Lexer;

const char* CONSOLE_RED = "\033[31m";
const char* CONSOLE_GREEN = "\033[32m";
const char* CONSOLE_RESET = "\033[0m";

int main(int argc, char **argv) {
    std::string inputFilename, outputFilename;
    int rd = 0;
    while (rd < argc) {
        std::string_view s(argv[rd]);
        if (s == "-o" && rd + 1 < argc) {
            rd++;
            outputFilename = argv[rd];
        } else {
            inputFilename = argv[rd];
        }
        rd++;
    }

    if (inputFilename.empty()) {
        cerr << "pl0cc: " << CONSOLE_RED << "Error" << CONSOLE_RESET << ": Input file not specified." << endl;
        return EXIT_FAILURE;
    }

    if (outputFilename.empty()) {
        cerr << "pl0cc: " << CONSOLE_RED << "Error" << CONSOLE_RESET << ": Output file not specified." << endl;
        return EXIT_FAILURE;
    }

    ifstream input(inputFilename);
    ofstream output(outputFilename);
    Lexer lexer;
    lexer.feedStream(input);
    input.close();

    if (!lexer.stopped()) {
        cerr << "pl0cc: " << CONSOLE_RED << "Error" << CONSOLE_RESET << ": Lexer hasn't stopped." << endl;
        return EXIT_FAILURE;
    }

    while (!lexer.tokenEmpty()) {
        auto token = lexer.takeToken();
        output << token.serialize() << endl;
    }
    output << flush;
    output.close();

    cerr << "pl0cc completed with ";
    if (lexer.errorCount() == 0) {
        cerr << CONSOLE_GREEN << "0" << CONSOLE_RESET << " errors occurred." << endl;
    } else {
        cerr << CONSOLE_RED << lexer.errorCount() << CONSOLE_RESET << " errors occurred." << endl;
        cerr << endl;
        for (size_t i=0; i<lexer.errorCount(); i++) {
            Lexer::ErrorReport report = lexer.errorReportAt(i);

            string srcLine = lexer.sourceLine(report.lineNumber());
            stringstream hintLine;
            for (int idx = 0; idx < srcLine.size(); idx++) {
                if (idx == report.columnNumber()) {
                    hintLine << CONSOLE_RED;
                }
                if (idx == report.columnNumber() + report.tokenLength()) {
                    hintLine << CONSOLE_RESET;
                }
                hintLine << srcLine[idx];
            }

            cerr << "Error " << i << ": " << endl;
            cerr << "---------------------" << endl;
            cout << report.lineNumber()+1 << " |\t" << hintLine.str() << endl << endl;
        }
    }

    return EXIT_SUCCESS;
}