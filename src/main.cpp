#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <string_view>

#include "lexer.hpp"

using namespace std;
using pl0cc::Lexer, pl0cc::Token;

const char* CONSOLE_RED = "\033[31m";
const char* CONSOLE_GREEN = "\033[32m";
const char* CONSOLE_RESET = "\033[0m";

int main(int argc, char **argv) {
    std::string inputFilename, outputFilename;
    int rd = 1;
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

    auto absoluteInputPath = filesystem::absolute(inputFilename);
    ifstream input(inputFilename);
    Lexer lexer;
    lexer.feedStream(input);
    input.close();

    if (!lexer.stopped()) {
        cerr << "pl0cc: " << CONSOLE_RED << "Error" << CONSOLE_RESET << ": Lexer hasn't stopped." << endl;
        return EXIT_FAILURE;
    }

    cerr << "pl0cc completed with ";
    if (lexer.errorCount() == 0) {
        cerr << CONSOLE_GREEN << "0" << CONSOLE_RESET << " errors occurred." << endl;

        ofstream output(outputFilename);
        while (!lexer.tokenEmpty()) {
            auto token = lexer.takeToken();
            output << token.serialize() << endl;
        }
        output << flush;
        output.close();
    } else {
        cerr << CONSOLE_RED << lexer.errorCount() << CONSOLE_RESET << " errors occurred." << endl;
        cerr << endl;
        for (size_t i=0; i<lexer.errorCount(); i++) {
            Lexer::ErrorReport report = lexer.errorReportAt(i);

            string srcFile = absoluteInputPath.string() + ":" + to_string(report.lineNumber() + 1) + ":" + to_string(report.columnNumber() + 1);
            cerr << "Error " << (i+1) << " at " << srcFile << ": " << std::endl;
            report.reportErrorTo(cerr);
        }
    }

    return EXIT_SUCCESS;
}