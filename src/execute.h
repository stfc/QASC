/**
 * @copyright Copyright (c) 2025-2026
 * UK Research and Innovation,
 * Science and Technology Facilities Council,
 * Hartree Centre
 **/

#ifndef QASC_EXECUTE_H
#define QASC_EXECUTE_H

#include <string>

void run(const std::string& cppCompiler,
         const std::string& srcFile,
         const std::string& outFile,
         const std::string& libPath,
         const std::string& libs,
         const std::string& includePath,
         const std::string& rpaths,
         const bool isFormatting,
         const bool isVerbose);

#endif    // defined QASC_EXECUTE_H
