/**
 * @copyright Copyright (c) 2025-2026
 * UK Research and Innovation,
 * Science and Technology Facilities Council,
 * Hartree Centre
 **/

#include "collectErrorStrategy.h"
#include "compiler.h"
#include "qasm3Lexer.h"
#include "qasm3Parser.h"
#include "utils.h"

#include <chrono>
#include <format>
#include <fstream>
#include <string>

void run(const std::string& cppCompiler,
         const std::string& srcFile,
         const std::string& outFile,
         const std::string& libPath,
         const std::string& libs,
         const std::string& includePath,
         const std::string& rpaths,
         const bool isFormatting,
         const bool isVerbose) {
    using std::chrono::duration;
    using std::chrono::duration_cast;
    using std::chrono::high_resolution_clock;
    using std::chrono::milliseconds;

    using namespace antlr4;
    using namespace antlr4cpp;

    auto t1 = high_resolution_clock::now();
    std::ifstream stream(srcFile);
    if (!stream.is_open()) {
        throw std::runtime_error(
            std::format("Failed to open source qasm file: {}.", srcFile));
    }

    ANTLRInputStream input(stream);
    qasm3Lexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    qasm3Parser parser(&tokens);
    stream.close();
    auto error_strat = std::make_shared<CollectErrorStrategy>();
    parser.setErrorHandler(error_strat);
    qasm3Parser::ProgramContext* tree = nullptr;
    tree = parser.program();
    CollectErrorStrategy* errorStrat =
        dynamic_cast<CollectErrorStrategy*>(parser.getErrorHandler().get());
    errorStrat->reportErrors();

    Compiler compiler(srcFile, outFile);

    compiler.visitProgram(tree);
    auto t2 = high_resolution_clock::now();

    duration<double, std::milli> ms_double = t2 - t1;
    if (isVerbose) {
        Utils::print(std::format("Frontend timing: {}ms", ms_double.count()));
    }

    t1 = high_resolution_clock::now();
    compiler.compile(cppCompiler, outFile, libPath, libs, includePath, rpaths,
                     isFormatting, isVerbose);
    t2 = high_resolution_clock::now();
    ms_double = t2 - t1;
    if (isVerbose) {
        Utils::print(std::format("Backend timing: {}ms", ms_double.count()));
    }
}
