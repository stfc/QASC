/**
 * @copyright Copyright (c) 2025-2026
 * UK Research and Innovation,
 * Science and Technology Facilities Council,
 * Hartree Centre
 **/

#ifndef QASC_UTILS_H
#define QASC_UTILS_H

#include <regex>
#include <string>
#include <string_view>

struct Output {
    std::string type{""};
    std::string varName{""};
    int numDims{0};
};

using Input = Output;

namespace Utils {

std::string externalErrorMessage(const std::string& msg);

void print(std::string_view str);

std::string createNDimVectorStr(const int numDim, const std::string& type);

template <typename T>
std::string replacePattern(const std::string& expr,
                           const std::regex& pattern,
                           T&& replaceCall) {
    std::string result;
    result.reserve(expr.length());

    size_t lastPos = 0;

    auto matchesStart = std::sregex_iterator(expr.begin(), expr.end(), pattern);
    auto matchesEnd = std::sregex_iterator();

    for (auto it = matchesStart; it != matchesEnd; ++it) {
        std::smatch match = *it;

        // get anything from prev match
        result.append(expr, lastPos, match.position() - lastPos);

        const std::string rewritten = replaceCall(match);
        result.append(rewritten);

        // move to next
        lastPos = match.position() + match.length();
    }

    // append what's left
    result.append(expr, lastPos, std::string::npos);
    return result;
}

std::string lookupBuiltInLiteral(std::string_view literal);

std::string replaceBuiltInLiteral(std::string& str,
                                  const std::string& from,
                                  const std::string& to);

std::string replaceAllBuiltInLiterals(std::string& str);

std::string replacePowerOp(const std::string& text);

std::string replaceAllCastExpr(const std::string& expr);

std::string replaceArrayCommaExpr(std::string& varName);

std::string replaceSlice(const std::string& expr);

std::string replaceImaginaryLiteral(const std::string& expr);

std::string replaceUnderscoresInNumbers(const std::string& expr);

std::string replaceTimeUnits(const std::string& expr);

std::string replaceQuregSlice(std::string& expr);

std::string processExpression(std::string& expr);

}    // namespace Utils

#endif    // !QASC_UTILS_H
