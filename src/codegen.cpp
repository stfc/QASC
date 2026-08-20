/**
 * @copyright Copyright (c) 2025-2026
 * UK Research and Innovation,
 * Science and Technology Facilities Council,
 * Hartree Centre
 **/

#include "codegen.h"
#include "utils.h"

#include <algorithm>
#include <array>
#include <format>
#include <stdexcept>
#include <utility>

namespace Generator {
Instr emitDepsHeaders() {
    return "#include \"quest.h\"\n#include \"argparse.hpp\"\n#include "
           "<vector>\n#include <type_traits>\n#include "
           "<fstream>\n#include <iostream>\n#include <cmath>\n#include "
           "<algorithm>\n#include \"qasc_gates.h\"\n\n";
}

Instr emitIncludeCode(StrView headerDef, Instr&& code) {
    return std::format(
        "#ifndef {}_HEADER\n"
        "#define {}_HEADER\n"
        "{}\n#endif // defined {}_HEADER\n",
        headerDef, headerDef, code, headerDef);
}

Instr emitBreakStmt() {
    return "break;\n";
}

Instr emitContinueStmt() {
    return "continue;\n";
}

Instr emitEndStmt() {
    return "exit(0);\n";
}

Instr emitLeftBracket() {
    return "{\n";
}

Instr emitRightBracket() {
    return "}\n";
}

Instr emitLeftParen() {
    return "(";
}

Instr emitRightParen() {
    return ")";
}

Instr emitForStmt(StrView indexId,
                  StrView start,
                  StrView end,
                  StrView increment) {
    // NOTE: range expression is inclusive on [start, end]
    std::string compOp = "<";
    std::string endOffset = " + 1";
    if (increment[0] == '-') {
        compOp = ">=";
        endOffset = "";
    }

    return std::format("for (auto {} = {}; {} {} {} {}; {} += {})", indexId,
                       start, indexId, compOp, end, endOffset, indexId,
                       increment);
}

Instr emitForStmt(StrView indexId, StrView vals) {
    if (vals.starts_with("{") && vals.ends_with("}")) {
        // it is a set expression
        return std::format("for (auto v = std::vector{}; auto& {} : v)", vals,
                           indexId);
    } else {
        // it is range based iteration
        return std::format("for (auto& {} : {})", indexId, vals);
    }
}

Instr emitIfStmt(StrView expression) {
    return std::format("if ({})\n", expression);
}

Instr emitElseStmt() {
    return "else\n";
}

Instr emitReturnStmt(StrView expression) {
    return std::format("return {};\n", expression);
}

Instr emitWhileStmt(StrView expression) {
    return std::format("while ({})\n", expression);
}

Instr emitSwitchStmt(StrView expression) {
    return std::format("switch ({})\n", expression);
}

Instr emitCaseStmt(StrView expression) {
    return std::format("case {}: ", expression);
}

Instr emitDefaultStmt() {
    return "default: ";
}

Instr emitGateCallStmt(StrView gateName, StrView qregName, StrView target) {
    const std::array<std::string, 6> availGates{"applyPauliX", "applyPauliY",
                                                "applyPauliZ", "applyHadamard",
                                                "applyS",      "applyT"};

    if (std::find(availGates.begin(), availGates.end(), gateName) ==
        availGates.end()) {
        throw std::runtime_error(
            std::format("{} is not a single qubit gate.", gateName));
    }
    return std::format("{}({}, {});\n", gateName, qregName, target);
}

Instr emitGateCallStmt(StrView gateName,
                       StrView qregName,
                       StrView target,
                       StrView ctrlOrAngle) {
    const std::array<std::string, 4> rotationGates{
        "applyRotateX", "applyRotateY", "applyRotateZ", "applyPhaseShift"};

    const std::array<std::string, 7> ctrlGates{"applyControlledPauliX",
                                               "applyControlledPauliY",
                                               "applyControlledPauliZ",
                                               "applyControlledHadamard",
                                               "applyControlledS",
                                               "applyControlledT",
                                               "applySwap"};

    if (std::find(rotationGates.begin(), rotationGates.end(), gateName) !=
        rotationGates.end()) {
        return std::format("{}({}, {}, {});\n", gateName, qregName, target,
                           ctrlOrAngle);
    }

    if (std::find(ctrlGates.begin(), ctrlGates.end(), gateName) !=
        ctrlGates.end()) {
        return std::format("{}({}, {}, {});\n", gateName, qregName, ctrlOrAngle,
                           target);
    }

    throw std::runtime_error(
        std::format("{} is not a rotation or control gate.", gateName));
}

Instr emitGateCallStmt(StrView gateName,
                       StrView qregName,
                       StrView target,
                       StrView ctrl,
                       StrView angle) {
    const std::array<std::string, 5> ctrlRotGates{
        "applyControlledRotateX", "applyControlledRotateY",
        "applyControlledRotateZ", "applyTwoQubitPhaseShift",
        "applyControlledSwap",
    };

    if (gateName == "applyMultiControlledPauliX") {
        return std::format("{}({}, ", gateName, qregName) + "{" +
               std::format("{}, {}", ctrl, target) + "}" +
               std::format(", {});\n", angle);
    }

    if (std::find(ctrlRotGates.begin(), ctrlRotGates.end(), gateName) ==
        ctrlRotGates.end()) {
        throw std::runtime_error(
            std::format("{} is not a controlled rotation gate.", gateName));
    }

    return std::format("{}({}, {}, {}, {});\n", gateName, qregName, ctrl,
                       target, angle);
}

Instr emitGateCallStmt(StrView gateName,
                       StrView qregName,
                       StrView target,
                       StrView angle1,
                       StrView angle2,
                       StrView angle3,
                       bool isInverse) {
    if (isInverse) {
        std::swap(angle2, angle3);
    }
    if (gateName == "U") {
        return std::format(
            "applyPhaseShift({}, {}, {});\n"
            "applyRotateY({}, {}, {});\n"
            "applyPhaseShift({}, {}, {});\n",
            qregName, target, angle2, qregName, target, angle1, qregName,
            target, angle3);
    } else {
        throw std::runtime_error(std::format("{} is not a U gate.", gateName));
    }
}

Instr emitGateCallStmt(StrView gateName,
                       StrView qregName,
                       StrView target,
                       StrView ctrl,
                       StrView angle1,
                       StrView angle2,
                       StrView angle3,
                       StrView angle4) {
    if (gateName == "cu" || gateName == "cU") {
        return std::format(
            "applyTwoQubitPhaseShift({}, {}, {}, {});\n"
            "applyPhaseShift({}, {}, {});\n"
            "applyControlledRotateY({}, {}, {}, {});\n"
            "applyTwoQubitPhaseShift({}, {}, {}, {});\n",
            qregName, ctrl, target, angle2, qregName, target, angle4, qregName,
            ctrl, target, angle1, qregName, ctrl, target, angle3);
    } else {
        throw std::runtime_error(
            std::format("{} is not a controlled-U gate.", gateName));
    }
}

Instr emitGPhaseStmt(StrView numQubits, StrView qregName, StrView angle) {
    return std::format("for (auto i = 0; i < {}; ++i)\n", numQubits) + "{\n" +
           std::format("applyPhaseShift({}, i, {});\n", qregName, angle) +
           "}\n";
}

Instr emitResetStmt(StrView qregName) {
    return std::format("initZeroState({});\n", qregName);
}

Instr emitResetStmt(StrView qregName, StrView target) {
    auto statement{
        std::format("if (applyQubitMeasurement({}, {})) ", qregName, target)};
    statement +=
        "{" + std::format(" applyPauliX({}, {}); ", qregName, target) + "}\n";
    return statement;
}

Instr emitAliasDecl(StrView aliasName, StrView expression) {
    return std::format("auto {} = {};\n", aliasName, expression);
}

Instr emitClassicalDeclStmt(StrView type, StrView varName, StrView expression) {
    if (expression == "") {
        return std::format("{} {};\n", type, varName);
    }

    if (expression.starts_with("(")) {
        // emit code for arrays generated earlier.
        return std::format("{} {}{};\n", type, varName, expression);
    } else if (expression.starts_with("{") || type.starts_with("std::vector")) {
        // emit code for arrays with declarationExpression
        return std::format("{} {} = {};\n", type, varName,
                           std::string(expression));
    }

    return std::format("{} {} = {};\n", type, varName, expression);
}

Instr emitClassicalDeclFront(StrView type, StrView varName) {
    return std::format("{} {}", type, varName);
}

Instr emitClassicalDeclBack() {
    return ";\n";
}

Instr emitQuantumDeclStmt(StrView qregName, StrView expression) {
    return std::format("Qureg {} = createQureg({});\n", qregName, expression);
}

Instr emitQuestInit() {
    return "if (!isQuESTEnvInit()) {\ninitQuESTEnv();\n}\n";
}

Instr emitQuestFree() {
    return "finalizeQuESTEnv();\n";
}

Instr emitConstMod(bool startedMain) {
    // generally assume that constant declarations in the global scope
    // should be compile-time constants and constant declarations inside main
    // body should be runtime readonly
    if (startedMain) {
        return "const ";
    } else {
        return "constexpr ";
    }
}

Instr emitIoDeclStmt(StrView type, StrView varName, StrView expression) {
    std::string value{expression};
    if (value != "" && !type.starts_with("std::vector")) {
        value = " = " + value;
    }
    return std::format("{} {}{};\n", type, varName, value);
}

Instr emitFuncDefStmt(StrView returnType, StrView fnName, StrView args) {
    return std::format("{} {}({})\n", returnType, fnName, args);
}

Instr emitOutput(StrView outputFilename, const std::vector<Output>& outputs) {
    std::string result{""};
    if (outputs.empty()) {
        return result;
    }

    result.append("if (shouldPrint()) {\n");

    const std::array<std::string, 6> tmpNames{"a", "b", "c", "d", "e", "f"};
    result.append("std::ofstream out(\"" + std::string(outputFilename) +
                  ".results\");\n");

    for (auto& output : outputs) {
        if (static_cast<size_t>(output.numDims) > tmpNames.size()) {
            throw std::runtime_error(
                std::format("Output array has too many dims: {}, max: {}",
                            output.numDims, tmpNames.size()));
        }

        result.append("out << \"" + output.varName + "\" << \": \";\n");
        if (output.numDims == 0) {
            result.append("out << " + output.varName + " << \"\\n\";\n");
            continue;
        }

        result.append("for (auto &" + tmpNames[0] + " : " + output.varName +
                      ") {\n");

        for (int i = 1; i < output.numDims; ++i) {
            result.append("for (auto &" + tmpNames[i] + " : " +
                          tmpNames[i - 1] + ") {\n");
        }
        result.append("out << " + tmpNames[output.numDims - 1] + " << \", \"" +
                      ";\n");

        for (int i = 1; i < output.numDims; ++i) {
            result.append("}\nout << \"\\n\";\n");
        }
        result.append("}\nout << \"\\n\";\n");
    }
    result.append("out.close();\n");
    result.append("}\n");
    return result;
}

Instr emitInputs(const std::vector<Input>& inputs) {
    std::string result{""};

    if (inputs.size() == 0) {
        return result;
    }

    result.append("argparse::ArgumentParser program(\"\");\n");
    for (const auto& input : inputs) {
        if (input.numDims > 1) {
            Utils::print(std::format(
                "skipping '{}' input variable as only scalars and 1D "
                "input vectors are allowed.",
                input.varName));
            return "";
        }
        std::string type{input.type};
        std::string scanSpecifier{};

        bool isAngle = type.ends_with(", true>");

        std::string typeInfo{""};
        if (!isAngle && type.find("unsigned int") != type.npos) {
            typeInfo = "uint";
            if (type.find("<unsigned int, 1>")) {
                typeInfo = "bool|bit";
            }

            scanSpecifier = ".scan<'i', unsigned int>()";
        } else if (!isAngle && type.find("int") != type.npos) {
            typeInfo = "int";
            scanSpecifier = ".scan<'i', int>()";
        } else if (type == "double" || isAngle) {
            typeInfo = "double|angle";
            scanSpecifier = ".scan<'g', double>()";
        } else {
            throw std::runtime_error(
                std::format("Unsupported input type {}", input.type));
        }

        if (input.numDims > 0) {
            type = Utils::createNDimVectorStr(input.numDims, type);
            typeInfo = "array[" + typeInfo + "]";
        }

        result.append(
            std::format("program.add_argument(\"--{}\")\n"
                        ".required()\n"
                        ".nargs(argparse::nargs_pattern::at_least_one)\n"
                        "{}\n"
                        ".help(\"qasm input value of type: {}\");\n",
                        input.varName, scanSpecifier, typeInfo));
    }

    result.append(
        "try\n{\nprogram.parse_args(argc, argv);\n}\ncatch (const "
        "std::exception& "
        "err)\n{\nstd::cerr << err.what() << std::endl;\nstd::cerr << "
        "program;\nreturn "
        "-1;\n}\n");

    for (const auto& input : inputs) {
        std::string type{input.type};
        if (input.numDims > 0) {
            type = Utils::createNDimVectorStr(input.numDims, type);
            result.append(
                emitIoDeclStmt(type, input.varName,
                               std::format(" = program.get<{}>(\"{}\")", type,
                                           input.varName)));
        } else {
            result.append(emitIoDeclStmt(
                type, input.varName,
                std::format("program.get<{}>(\"{}\")", type, input.varName)));
        }
    }
    return result;
}

Instr emitAssignmentStmt(StrView varName,
                         StrView assignOp,
                         StrView expression) {
    const std::array<std::string, 10> validOps{
        "=", "+=", "-=", "*=", "/=", "<<=", ">>=", "|=", "&=", "^="};

    if (std::find(validOps.begin(), validOps.end(), assignOp) ==
        validOps.end()) {
        throw std::runtime_error(
            std::format("{} is not a valid assignment operator.", assignOp));
    }

    return std::format("{} {} {};\n", varName, assignOp, expression);
}

Instr emitAssignmentFront(StrView varName, StrView assignOp) {
    const std::array<std::string, 10> validOps{
        "=", "+=", "-=", "*=", "/=", "<<=", ">>=", "|=", "&=", "^="};

    if (std::find(validOps.begin(), validOps.end(), assignOp) ==
        validOps.end()) {
        throw std::runtime_error(
            std::format("{} is not a valid assignment operator.", assignOp));
    }

    return std::format("{} {}", varName, assignOp);
}

Instr emitAssignmentBack() {
    return ";\n";
}

Instr emitCastFront(StrView type) {
    if (type == "float" || type == "angle") {
        type = "double";
    }
    return std::format("static_cast<{}>(", type);
}

Instr emitCastBack() {
    return ")";
}

Instr emitFuncCallStmt(StrView funcName, StrView argList) {
    if (funcName == "sizeof") {
        return std::format("{}({})", "qasmSizeof", argList);
    }
    return std::format("{}({})", funcName, argList);
}

Instr emitExprStmt(StrView expression) {
    return std::format("{}", expression);
}

Instr emitMeasurement(StrView qregName, StrView target) {
    return std::format("applyQubitMeasurement({}, {});\n", qregName, target);
}

// operators

Instr emitPlusOp() {
    return " + ";
}

Instr emitMinusOp() {
    return " - ";
}

Instr emitMultOp() {
    return " * ";
}

Instr emitDivOp() {
    return " / ";
}

Instr emitModOp() {
    return " % ";
}

Instr emitXorOp() {
    return " ^ ";
}

Instr emitCompOp(StrView op) {
    if (op == "<") {
        return " < ";
    } else if (op == ">") {
        return " > ";
    } else if (op == ">=") {
        return " >= ";
    } else if (op == "<=") {
        return " <= ";
    } else {
        throw std::runtime_error(
            std::format("Invalid comparison operator {}", op));
    }
}

Instr emitDoublePipeOp() {
    return " || ";
}

Instr emitPipeOp() {
    return " | ";
}

Instr emitBitshiftOp(StrView op) {
    if (op == "<<") {
        return " << ";
    } else if (op == ">>") {
        return " >> ";
    } else {
        throw std::runtime_error(
            std::format("Invalid bitshift operator {}", op));
    }
}

Instr emitAmpersandOp() {
    return " & ";
}

Instr emitEqualityOp(StrView op) {
    if (op == "==") {
        return " == ";
    } else if (op == "!=") {
        return " != ";
    } else {
        throw std::runtime_error(
            std::format("Invalid equality operator {}", op));
    }
}

Instr emitDoubleAmpersandOp() {
    return " && ";
}

Instr emitMinusUnary() {
    return "-";
}

Instr emitExclamationUnary() {
    return "!";
}

Instr emitTildeUnary() {
    return "~";
}

Instr emitPowOp(StrView lhs, StrView rhs) {
    return std::format("pow({}, {})", lhs, rhs);
}

Instr emitLiteral(StrView literal) {
    if (literal == "pi") {
        return "3.14159265358979323846";
    } else if (literal == "tau") {
        return "6.28318530717958647692";
    } else if (literal == "euler") {
        return "2.71828182845904523536";
    } else {
        return std::format("{}", literal);
    }
}

Instr emitScalarType(std::string scalarType) {
    auto first = scalarType.find("[");
    auto last = scalarType.find("]", first);
    if (first == scalarType.npos || last == scalarType.npos) {
        if (scalarType == "float" || scalarType == "angle") {
            return "double";
        }
        return scalarType;
    }

    scalarType.erase(first, last - first + 1);
    if (scalarType == "float" || scalarType == "angle") {
        return "double";
    }
    return scalarType;
}

Instr emitNdArray(int numDim, StrView type) {
    if (numDim < 0) {
        throw std::runtime_error(std::format(
            "error in emitNdArray, passed negative number of array dims: "
            "{}.",
            numDim));
    }

    if (numDim == 0) {
        return std::string(type);
    }

    return std::format("std::vector<{}>", emitNdArray(numDim - 1, type));
}

}    // namespace Generator
