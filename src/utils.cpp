/**
 * @copyright Copyright (c) 2025-2026
 * UK Research and Innovation,
 * Science and Technology Facilities Council,
 * Hartree Centre
 **/

#include "utils.h"
#include <array>
#include <cctype>
#include <cstddef>
#include <format>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace {
const std::regex exponentPattern(
    R"((((\w+)\[([^\]]+)\*\*([^\]]+)\]))|(-?[^(),<>=!+*-]+|\([^()]+\))\s*(\*\*)\s*(-?[^(),<>=!+*-]+|\([^()]+\)))");

const std::regex castPattern(
    R"(((int|uint|bit|bool|double|float|angle|complex|array\[[\w,]+\])(\[(\w+)?\])?\(([\w+\[\]\(\)*/-]+)\)))");

const std::regex slicePattern(
    R"(((\w+)(\[[\w+*(),/-]+\])*\s*(\[([\w+*(),/-]+):([\w+*(),/-]+)(:([\w+*(),/-]+))?\]|\[([\w+*(),/-]+)\])\s*(\[[\w+*(),/-]+\])*))");

const std::regex imaginaryLiteralPattern(R"(((\d+)\s*im))");

const std::regex underscorePattern(
    R"((((0[xXbBoO])|(\d+_))(([0-9a-fA-F]+)(_))+([0-9a-fA-F]+)))");

const std::regex sndUnderscorePattern(R"((([0-9a-fA-F]+)(_)))");

const std::regex timeUnitsPattern(R"((\d+)(s|ms|μs|us|ns|dt))");

const std::regex quregWithIdxPattern(
    R"(((\w+)(\[((\w+)\s*([\s+\-*\/\w+]*))\s*:\s*((\w+)\s*([\s+\-*\/\w+]*))\s*:?\s*((\w+)\s*([\s+\-*\/\w+]*))?\])))");

}    // namespace

std::string Utils::externalErrorMessage(const std::string& msg) {
    std::string error =
        std::string("\033[1;31m") + "external error: " + "\033[0m" + msg + "\n";

    return (error);
}

void Utils::print(std::string_view str) {
    std::cout << str << "\n";
}

std::string Utils::createNDimVectorStr(const int numDim,
                                       const std::string& type) {
    if (numDim == 0) {
        return type;
    }
    return "std::vector<" + createNDimVectorStr(numDim - 1, type) + ">";
}

std::string Utils::lookupBuiltInLiteral(std::string_view literal) {
    if (literal == "pi" || literal == "π") {
        return "3.14159265358979323846";
    } else if (literal == "tau") {
        return "6.28318530717958647692";
    } else if (literal == "euler") {
        return "2.71828182845904523536";
    } else if (literal == "sizeof") {
        return "qasmSizeof";
    } else {
        throw std::runtime_error(
            std::format("unsupported built in literal: {}", literal));
    }
}

std::string Utils::replaceBuiltInLiteral(std::string& str,
                                         const std::string& from,
                                         const std::string& to) {
    if (from.empty()) {
        return "";
    }

    size_t startPos = 0;

    while ((startPos = str.find(from, startPos)) != std::string::npos) {
        if (startPos != 0) {
            if (str[startPos - 1] == '_' || std::isalpha(str[startPos - 1])) {
                startPos += from.length();
                continue;
            }
        }

        str.replace(startPos, from.length(), to);
        startPos += to.length();
    }
    return str;
}

std::string Utils::replaceAllBuiltInLiterals(std::string& str) {
    for (const auto& builtIn : {"pi", "π", "tau", "euler", "sizeof"}) {
        Utils::replaceBuiltInLiteral(str, builtIn,
                                     lookupBuiltInLiteral(builtIn));
    }
    return str;
}

std::string Utils::replacePowerOp(const std::string& expr) {
    if (expr.find("**") == std::string::npos) {
        return expr;
    }

    std::string result;
    result.reserve(expr.length());

    size_t lastPos = 0;

    auto matchesStart =
        std::sregex_iterator(expr.begin(), expr.end(), exponentPattern);
    auto matchesEnd = std::sregex_iterator();

    for (auto it = matchesStart; it != matchesEnd; ++it) {
        std::smatch match = *it;

        // get anything from prev match
        result.append(expr, lastPos, match.position() - lastPos);

        std::string base = match[4].str();
        std::string exp = match[5].str();
        bool shouldAppendBracketOp = true;
        if (match[6] != "" && match[8].str() != "") {
            base = match[6].str();
            exp = match[8].str();
            shouldAppendBracketOp = false;
        }

        std::string rewritten = "pow(" + base + ", " + exp + ")";
        if (shouldAppendBracketOp) {
            rewritten = match[3].str() + "[" + rewritten + "]";
        }
        result.append(rewritten);

        // move to next
        lastPos = match.position() + match.length();
    }

    // append what's left
    result.append(expr, lastPos, std::string::npos);
    return result;
}

std::string Utils::replaceAllCastExpr(const std::string& expr) {
    constexpr std::array<std::string_view, 9> castKeyword = {
        "int",   "uint",  "bit",     "bool", "double",
        "float", "angle", "complex", "array"};

    if (std::any_of(castKeyword.begin(), castKeyword.end(),
                    [&](const auto& str) {
                        return expr.find(str) != std::string::npos;
                    })) {
        return replacePattern(
            expr, castPattern, [](const std::smatch& match) -> std::string {
                std::string type = match[2].str();
                std::string typeSize = match[4].str();
                std::string varName = match[5].str();

                if (typeSize != "") {
                    typeSize = ", " + typeSize;
                }

                if (type == "double" || type == "bool" || type == "float") {
                    return std::format("static_cast<{}>({})", type, varName);
                } else if (type == "bit" || type == "uint") {
                    type = "unsigned int";
                } else if (type == "angle") {
                    type = "unsigned int";
                    typeSize += ", true";
                } else if (type.starts_with("array")) {
                    throw std::runtime_error(
                        "\033[1;31merror: \033[0mcasting array types is not "
                        "supported\n");
                }

                return std::format("static_cast<QasmType<{}{}>>({})", type,
                                   typeSize, varName);
            });
    } else {
        return expr;
    }
}

// turns arr[x, y, z] to arr[x][y][z]
std::string Utils::replaceArrayCommaExpr(std::string& expr) {
    if (expr.find(",") == std::string::npos ||
        expr.find("[") == std::string::npos ||
        expr.find("]") == std::string::npos) {
        return expr;
    }

    std::string indexedVarName{};
    indexedVarName.reserve(expr.length());

    std::string::size_type lastPos = 0;
    std::string::size_type findPos;

    std::string::size_type lastLeftBracketPos = 0;
    auto leftBracketPos = expr.find("[", lastLeftBracketPos);

    if (leftBracketPos == std::string::npos) {
        return expr;
    }

    std::string::size_type lastRightBracketPos = 0;
    auto rightBracketPos = expr.find("]", lastRightBracketPos);

    std::string::size_type lastLeftParenthPos = 0;
    auto leftParenthPos = expr.find("(", lastLeftParenthPos);
    std::string::size_type lastRightParenthPos = 0;
    auto rightParenthPos = expr.find(")", lastRightParenthPos);

    // some examples
    // [a, b]
    // [a(c, d), c]
    // [a(b[a,c])]
    // a[c,b] , a[a,c]
    // a[b[1, 3]]
    // a[x, y, z]

    while (std::string::npos != (findPos = expr.find(",", lastPos))) {
        if ((std::string::npos != leftBracketPos &&
             std::string::npos != rightBracketPos) &&
            (findPos <= leftBracketPos || findPos >= rightBracketPos)) {
            indexedVarName.append(expr, lastPos, findPos - lastPos + 1);
            lastPos = findPos + 1;
            continue;
        }
        lastLeftBracketPos = leftBracketPos + 1;
        lastRightBracketPos = rightBracketPos + 1;
        leftBracketPos = expr.find("[", lastLeftBracketPos);
        rightBracketPos = expr.find("]", lastRightBracketPos);

        if (findPos > leftParenthPos && findPos < rightParenthPos) {
            bool skip = leftBracketPos > rightParenthPos;

            lastLeftParenthPos = leftParenthPos + 1;
            lastRightParenthPos = rightParenthPos + 1;
            leftParenthPos = expr.find("(", lastLeftParenthPos);
            rightParenthPos = expr.find(")", lastRightParenthPos);
            if (skip) {
                indexedVarName.append(expr, lastPos, findPos - lastPos + 1);
                lastPos = findPos + 1;
                continue;
            }
        }

        indexedVarName.append(expr, lastPos, findPos - lastPos);
        indexedVarName += "][";
        lastPos = findPos + 1;
    }
    indexedVarName += expr.substr(lastPos);

    expr = indexedVarName;
    return expr;
}

std::string Utils::replaceSlice(const std::string& expr) {
    if (expr.find(":") == std::string::npos) {
        return expr;
    }

    return replacePattern(
        expr, slicePattern, [&](const std::smatch& match) -> std::string {
            auto varName = match[2].str();
            auto preIndices = match[3].str();
            auto start = match[5].str();
            auto end = match[6].str();
            auto increment = match[8].str();
            auto postIndices = match[10].str();

            if (increment == "") {
                increment = "1";
            } else {
                std::swap(increment, end);
            }

            std::string newExpr{""};
            if (start == "" && end == "") {
                newExpr = match[1].str();    // handles index operator as well
            } else {
                newExpr =
                    std::format("getSlice({}{}, {}, {}, {}){}", varName,
                                preIndices, start, end, increment, postIndices);
            }
            return newExpr;
        });
}

std::string Utils::replaceImaginaryLiteral(const std::string& expr) {
    if (expr.find("im") == std::string::npos) {
        return expr;
    }

    return replacePattern(expr, imaginaryLiteralPattern,
                          [](const std::smatch& match) -> std::string {
                              std::string imagExpr = match[1].str();
                              std::string numberLiteral = match[2].str();

                              return numberLiteral + "_im";
                          });

    std::string result;
    result.reserve(expr.length());

    size_t lastPos = 0;

    auto matchesStart =
        std::sregex_iterator(expr.begin(), expr.end(), imaginaryLiteralPattern);
    auto matchesEnd = std::sregex_iterator();

    for (auto it = matchesStart; it != matchesEnd; ++it) {
        std::smatch match = *it;

        // get anything from prev match
        result.append(expr, lastPos, match.position() - lastPos);

        std::string imagExpr = match[1].str();
        std::string numberLiteral = match[2].str();

        std::string rewritten = numberLiteral + "_im";
        result.append(rewritten);

        // move to next
        lastPos = match.position() + match.length();
    }

    // append what's left
    result.append(expr, lastPos, std::string::npos);
    return result;
}

std::string Utils::replaceUnderscoresInNumbers(const std::string& expr) {
    if (expr.find("_") == std::string::npos) {
        return expr;
    }

    auto result = replacePattern(
        expr, underscorePattern, [&](const std::smatch& match) -> std::string {
            std::string header{""};
            if (match[2].str()[0] == '0') {
                header = match.str();
            }

            return header +
                   replacePattern(expr, sndUnderscorePattern,
                                  [](const std::smatch& match) -> std::string {
                                      return match[2];
                                  });
        });

    if (expr.size() < result.size()) {
        const auto pos = result.find(expr);
        if (pos != result.npos) {
            result = result.substr(pos + expr.length());
        }
    }
    return result;
}

std::string Utils::replaceTimeUnits(const std::string& expr) {
    constexpr std::array<std::string_view, 6> timeUnits = {"ms", "us", "μs",
                                                           "ns", "s",  "dt"};

    if (std::any_of(timeUnits.begin(), timeUnits.end(), [&](const auto& str) {
            return expr.find(str) != std::string::npos;
        })) {
        return replacePattern(expr, timeUnitsPattern,
                              [](const std::smatch& match) -> std::string {
                                  std::string result = match[1].str();
                                  const std::string unit = match[2].str();
                                  if (unit == "ms") {
                                      result += " * 1e-3";
                                  } else if (unit == "μs" || unit == "us") {
                                      result += " * 1e-6";
                                  } else if (unit == "ns") {
                                      result += " * 1e-9";
                                  }
                                  // ignore dt and s

                                  return result;
                              });
    } else {
        return expr;
    }
}

std::string Utils::replaceQuregSlice(std::string& expr) {
    if (expr.find(":") == std::string::npos) {
        return expr;
    }

    std::string result;
    result.reserve(expr.length());

    auto matchesStart =
        std::sregex_iterator(expr.begin(), expr.end(), quregWithIdxPattern);
    auto matchesEnd = std::sregex_iterator();

    size_t lastPos = 0;
    for (auto it = matchesStart; it != matchesEnd; ++it) {
        std::smatch match = *it;
        result.append(expr, lastPos, match.position() - lastPos);

        auto varName = match[2].str();
        auto start = match[4].str();
        auto end = match[7].str();
        auto increment = match[10].str();

        std::string rewritten;
        if (increment == "") {
            rewritten = std::format("{}.slice({}, {})", varName, start, end);
        } else {
            std::swap(end, increment);
            rewritten = std::format("{}.slice({}, {}, {})", varName, start, end,
                                    increment);
        }
        result.append(rewritten);
        lastPos = match.position() + match.length();
    }

    result.append(expr, lastPos, std::string::npos);
    return result;
}

std::string Utils::processExpression(std::string& expr) {
    expr = Utils::replaceUnderscoresInNumbers(expr);
    expr = Utils::replaceTimeUnits(expr);
    expr = Utils::replaceAllBuiltInLiterals(expr);
    expr = Utils::replaceArrayCommaExpr(expr);
    expr = Utils::replacePowerOp(expr);
    expr = Utils::replaceAllCastExpr(expr);
    expr = Utils::replaceImaginaryLiteral(expr);
    expr = Utils::replaceSlice(expr);
    return expr;
}
