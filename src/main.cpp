/**
 * @copyright Copyright (c) 2025-2026
 * UK Research and Innovation,
 * Science and Technology Facilities Council,
 * Hartree Centre
 **/

#include "argparse.hpp"
#include "config_paths.h"
#include "execute.h"
#include "src/utils.h"

#include <algorithm>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("qasc");
    program.add_argument("-cxx").required().help(
        "specify standard C++ compiler.");

    program.add_argument("-i", "--input")
        .required()
        .help("specify the input file.");

    program.add_argument("-o", "--output")
        .required()
        .help("specify the output file.");

    program.add_argument("-L")
        .default_value<std::vector<std::string>>({})
        .append()
        .help("specify library paths.");

    program.add_argument("-l")
        .default_value<std::vector<std::string>>({})
        .append()
        .help("specify libraries.");

    program.add_argument("-I")
        .default_value<std::vector<std::string>>({})
        .append()
        .help("specify include paths.");

    program.add_argument("-F")
        .help("set output cpp formating (requires clang-format).")
        .flag();

    program.add_argument("-V", "--verbose")
        .help("set qasc program execution to verbose mode.")
        .flag();

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return -1;
    }

    auto cppCompiler = program.get<std::string>("-cxx");
    auto srcFile = program.get<std::string>("-i");
    auto outFile = program.get<std::string>("-o");
    auto libPath = program.get<std::vector<std::string>>("-L");
    auto libs = program.get<std::vector<std::string>>("-l");
    auto includePath = program.get<std::vector<std::string>>("-I");
    bool isFormatting = program.get<bool>("-F");
    bool isVerbose = program.get<bool>("-V");

    if (std::string(DEPS_QUEST_INCLUDE_DIR) == "") {
        throw std::runtime_error(Utils::externalErrorMessage(
            "include path to QuEST was not set correctly. Please review your "
            "build configuration."));
    }
    const char* envInclude = std::getenv("DEPS_QUEST_INCLUDE_DIR");
    includePath.emplace_back(envInclude ? envInclude : DEPS_QUEST_INCLUDE_DIR);
    includePath.emplace_back(DEPS_QASC_INCLUDE_DIR);

    if (std::string(DEPS_QUEST_LIB_DIR) == "") {
        throw std::runtime_error(Utils::externalErrorMessage(
            "library path to QuEST was not set correctly. Please review your "
            "build configuration."));
    }
    const char* envLib = std::getenv("DEPS_QUEST_LIB_DIR");
    libPath.emplace_back(envLib ? envLib : DEPS_QUEST_LIB_DIR);

    std::string includePathStr = "";
    std::for_each(includePath.begin(), includePath.end(),
                  [&](auto& el) { includePathStr += " -I " + el; });

    std::string libPathStr = "";
    std::for_each(libPath.begin(), libPath.end(),
                  [&](auto& el) { libPathStr += " -L " + el; });

    std::string libStr = "";
    std::for_each(libs.begin(), libs.end(),
                  [&](auto& el) { libStr += " -l" + el; });

    std::string rpathPath;
    std::for_each(libPath.begin(), libPath.end(),
                  [&](auto& el) { rpathPath += " -Wl,-rpath " + el; });

    if (isVerbose) {
        Utils::print(includePathStr);
        Utils::print(libPathStr);
        Utils::print(libStr);
        Utils::print(rpathPath);
    }

    try {
        run(cppCompiler, srcFile, outFile, libPathStr, libStr, includePathStr,
            rpathPath, isFormatting, isVerbose);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        return -1;
    }

    return 0;
}
