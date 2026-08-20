/**
 * @copyright Copyright (c) 2025
 * UK Research and Innovation,
 * Science and Technology Facilities Council,
 * Hartree Centre
 **/

// #include "codegen.h"
// #include "utils.h"
#include "src/codegen.h"
#include "src/utils.h"

#include "doctest.h"

using namespace Generator;

TEST_CASE("testing emit of dependency headers.") {
    CHECK(emitDepsHeaders() ==
          "#include \"quest.h\"\n#include \"argparse.hpp\"\n#include "
          "<vector>\n#include <type_traits>\n#include <fstream>\n#include "
          "<iostream>\n#include <cmath>\n#include <algorithm>\n#include "
          "\"qasc_gates.h\"\n\n");
}

TEST_CASE("testing emit of include code.") {
    CHECK(emitIncludeCode("EXAMPLE_H", "int x = 5;") ==
          "#ifndef EXAMPLE_H_HEADER\n"
          "#define EXAMPLE_H_HEADER\n"
          "int x = 5;\n"
          "#endif // defined EXAMPLE_H_HEADER\n");
}

TEST_CASE("testing emit of break statement.") {
    CHECK(emitBreakStmt() == "break;\n");
}

TEST_CASE("testing emit of continue statement.") {
    CHECK(emitContinueStmt() == "continue;\n");
}

TEST_CASE("testing emit of end statement.") {
    CHECK(emitEndStmt() == "exit(0);\n");
}

TEST_CASE("testing emit of left bracket") {
    CHECK(emitLeftBracket() == "{\n");
}

TEST_CASE("testing emit of right bracket") {
    CHECK(emitRightBracket() == "}\n");
}

TEST_CASE("testing emit of left paren") {
    CHECK(emitLeftParen() == "(");
}

TEST_CASE("testing emit of right paren") {
    CHECK(emitRightParen() == ")");
}

TEST_CASE("testing emit of for statement.") {
    SUBCASE("emitting range-based for loop.") {
        CHECK(emitForStmt("i", "0", "20", "1") ==
              "for (auto i = 0; i < 20  + 1; i += 1)");
        CHECK(emitForStmt("i", "0", "20", "3") ==
              "for (auto i = 0; i < 20  + 1; i += 3)");
        CHECK(emitForStmt("e", "es") == "for (auto& e : es)");
    }
    SUBCASE("emitting set-based for loop.") {
        SUBCASE("set-based for loop is generated as intended.") {
            CHECK(emitForStmt("e", "{1, 2, 3}") ==
                  "for (auto v = std::vector{1, 2, 3}; auto& e : v)");
        }

        SUBCASE("for loop accepts empty sets.") {
            CHECK(emitForStmt("e", "{}") ==
                  "for (auto v = std::vector{}; auto& e : v)");
        }
    }
}

TEST_CASE("testing emit of if-else statement.") {
    SUBCASE("emits pure if statement.") {
        CHECK(emitIfStmt("1 < 4") == "if (1 < 4)\n");
    }

    SUBCASE("emits else statement.") {
        CHECK(emitElseStmt() == "else\n");
    }
}

TEST_CASE("testing emit of return statement.") {
    CHECK(emitReturnStmt("4 * 25") == "return 4 * 25;\n");
}

TEST_CASE("testing emit of while statement.") {
    CHECK(emitWhileStmt("42 * 1 < 4") == "while (42 * 1 < 4)\n");
}

TEST_CASE("testing emit of switch statement.") {
    CHECK(emitSwitchStmt("4") == "switch (4)\n");
}

TEST_CASE("testing emit of case statement.") {
    CHECK(emitCaseStmt("4") == "case 4: ");
    CHECK(emitDefaultStmt() == "default: ");
}

TEST_CASE("testing emit of default statement.") {
    CHECK(emitDefaultStmt() == "default: ");
}

TEST_CASE("testing emit of gate call statement.") {
    SUBCASE("testing call to single qubit gate.") {
        CHECK(emitGateCallStmt("applyPauliX", "qubits", "42") ==
              "applyPauliX(qubits, 42);\n");
        CHECK(emitGateCallStmt("applyHadamard", "qubits", "-1") ==
              "applyHadamard(qubits, -1);\n");
        CHECK_THROWS_WITH(emitGateCallStmt("applyRotateY", "qubits", "42"),
                          "applyRotateY is not a single qubit gate.");
        CHECK_THROWS_WITH(
            emitGateCallStmt("applyControlledPauliZ", "qubits", "42"),
            "applyControlledPauliZ is not a single qubit gate.");
    }

    SUBCASE("testing call to single qubit rotation gate.") {
        CHECK(emitGateCallStmt("applyRotateX", "qubits", "42", "0.11241") ==
              "applyRotateX(qubits, 42, 0.11241);\n");
        CHECK_THROWS_WITH(
            emitGateCallStmt("applyHadamard", "qubits", "-1", "42"),
            "applyHadamard is not a rotation or control gate.");
    }

    SUBCASE("testing call to two qubit gate.") {
        CHECK(emitGateCallStmt("applyControlledPauliY", "qubits", "42", "24") ==
              "applyControlledPauliY(qubits, 24, 42);\n");
    }

    SUBCASE("testing call to two qubit rotation gate.") {
        CHECK(emitGateCallStmt("applyControlledRotateY", "qubits", "42", "24",
                               "0.161") ==
              "applyControlledRotateY(qubits, 24, 42, 0.161);\n");

        CHECK_THROWS_WITH(
            emitGateCallStmt("applyHadamard", "qubits", "-1", "24", "0.1251"),
            "applyHadamard is not a controlled rotation gate.");

        CHECK_THROWS_WITH(
            emitGateCallStmt("applyControlledPauliZ", "qubits", "-1", "24",
                             "0.1251"),
            "applyControlledPauliZ is not a controlled rotation gate.");

        CHECK_THROWS_WITH(
            emitGateCallStmt("applyRotateZ", "qubits", "-1", "24", "0.1251"),
            "applyRotateZ is not a controlled rotation gate.");
    }

    SUBCASE("testing bad call to quantum gate.") {
        CHECK_THROWS_WITH(
            emitGateCallStmt("asg", "qubits", "-1", "24", "0.1251"),
            "asg is not a controlled rotation gate.");

        CHECK_THROWS_WITH(emitGateCallStmt("asg", "qubits", "-1", "24"),
                          "asg is not a rotation or control gate.");

        CHECK_THROWS_WITH(emitGateCallStmt("asg", "qubits", "-1"),
                          "asg is not a single qubit gate.");
    }
}

TEST_CASE("testing emit of reset statement.") {
    CHECK(emitResetStmt("qubits") == "initZeroState(qubits);\n");
    CHECK(
        emitResetStmt("qubits", "1") ==
        "if (applyQubitMeasurement(qubits, 1)) { applyPauliX(qubits, 1); }\n");
}

TEST_CASE("testing emit of classical declaration statement.") {
    CHECK(emitClassicalDeclStmt("int", "bar", "") == "int bar;\n");
    CHECK(emitClassicalDeclStmt("int", "bar", "35") == "int bar = 35;\n");
    CHECK(emitClassicalDeclStmt("std::vector<int>", "bar", "") ==
          "std::vector<int> bar;\n");
    CHECK(emitClassicalDeclStmt("std::vector<int>", "bar", "{1, 1, 3}") ==
          "std::vector<int> bar = {1, 1, 3};\n");
}

TEST_CASE("testing emit of classical declaration front.") {
    CHECK(emitClassicalDeclFront("int", "bar") == "int bar");
    CHECK(emitClassicalDeclFront("std::vector<int>", "bar") ==
          "std::vector<int> bar");
}

TEST_CASE("testing emit of classical declaration back.") {
    CHECK(emitClassicalDeclBack() == ";\n");
}

TEST_CASE("testing emit of quantum declaration statement.") {
    CHECK(emitQuantumDeclStmt("qubits", "numQubits") ==
          "Qureg qubits = createQureg(numQubits);\n");
}

TEST_CASE("testing emit of quest initialization and finalization.") {
    CHECK(emitQuestInit() == "if (!isQuESTEnvInit()) {\ninitQuESTEnv();\n}\n");
    CHECK(emitQuestFree() == "finalizeQuESTEnv();\n");
}

TEST_CASE("testing emit of const modifier.") {
    CHECK(emitConstMod(false) == "constexpr ");
    CHECK(emitConstMod(true) == "const ");
}

TEST_CASE("testing emit of I/O declaration statement.") {
    CHECK(emitIoDeclStmt("double", "bar", "") == "double bar;\n");
    CHECK(emitIoDeclStmt("double", "bar", "32") == "double bar = 32;\n");
    CHECK(emitIoDeclStmt("std::vector<double>", "bar", "(0)") ==
          "std::vector<double> bar(0);\n");
    CHECK(emitIoDeclStmt("std::vector<std::vector<double>>", "bar",
                         "(4, std::vector<double>(0))") ==
          "std::vector<std::vector<double>> bar(4, std::vector<double>(0));\n");
}

TEST_CASE("testing emit of function definition statement.") {
    CHECK(emitFuncDefStmt("void", "foo", "") == "void foo()\n");
    CHECK(emitFuncDefStmt("int", "foo", "") == "int foo()\n");
}

TEST_CASE("testing emit of the output variable.") {
    std::vector<Output> vec{{"int", "measurements", 2}};
    CHECK(emitOutput("foo", vec) ==
          "if (shouldPrint()) {\n"
          "std::ofstream out(\"foo.results\");\n"
          "out << \"measurements\" << \": \";\n"
          "for (auto &a : measurements) {\n"
          "for (auto &b : a) {\n"
          "out << b << \", \";\n}\n"
          "out << \"\\n\";\n}\n"
          "out << \"\\n\";\n"
          "out.close();\n"
          "}\n");

    std::vector<Output> vec2{{}};
    CHECK(emitOutput("foo", vec2) ==
          "if (shouldPrint()) {\n"
          "std::ofstream out(\"foo.results\");\n"
          "out << \"\" << \": \";\n"
          "out <<  << \"\\n\";\n"
          "out.close();\n"
          "}\n");

    std::vector<Output> vec3{};
    CHECK(emitOutput("foo", vec3) == "");
}

TEST_CASE("testing emit of the input variables.") {
    std::vector<Input> vec{{"int", "inputs", 1}};
    CHECK(emitInputs(vec) ==
          "argparse::ArgumentParser program(\"\");\n"
          "program.add_argument(\"--inputs\")\n"
          ".required()\n"
          ".nargs(argparse::nargs_pattern::at_least_one)\n"
          ".scan<'i', int>()\n"
          ".help(\"qasm input value of type: array[int]\");\n"
          "try\n{\n"
          "program.parse_args(argc, argv);\n}\n"
          "catch (const std::exception& err)\n{\n"
          "std::cerr << err.what() << std::endl;\n"
          "std::cerr << program;\n"
          "return -1;\n}\n"
          "std::vector<int> inputs = "
          "program.get<std::vector<int>>(\"inputs\");\n");

    std::vector<Input> vec2{{"double", "inputs", 2}};
    CHECK(emitInputs(vec2) == "");

    std::vector<Input> vec3{{"int", "inputs", 0}};
    CHECK(emitInputs(vec3) ==
          "argparse::ArgumentParser program(\"\");\n"
          "program.add_argument(\"--inputs\")\n"
          ".required()\n"
          ".nargs(argparse::nargs_pattern::at_least_one)\n"
          ".scan<'i', int>()\n"
          ".help(\"qasm input value of type: int\");\n"
          "try\n{\n"
          "program.parse_args(argc, argv);\n}\n"
          "catch (const std::exception& err)\n{\n"
          "std::cerr << err.what() << std::endl;\n"
          "std::cerr << program;\n"
          "return -1;\n}\n"
          "int inputs = "
          "program.get<int>(\"inputs\");\n");

    std::vector<Input> vec4{{"int", "inputs", 1}};
    CHECK(emitInputs(vec4) ==
          "argparse::ArgumentParser program(\"\");\n"
          "program.add_argument(\"--inputs\")\n"
          ".required()\n"
          ".nargs(argparse::nargs_pattern::at_least_one)\n"
          ".scan<'i', int>()\n"
          ".help(\"qasm input value of type: array[int]\");\n"
          "try\n{\n"
          "program.parse_args(argc, argv);\n}\n"
          "catch (const std::exception& err)\n{\n"
          "std::cerr << err.what() << std::endl;\n"
          "std::cerr << program;\n"
          "return -1;\n}\n"
          "std::vector<int> inputs = "
          "program.get<std::vector<int>>(\"inputs\");\n");

    std::vector<Input> vec5{{}};
    CHECK_THROWS_WITH(emitInputs(vec5), "Unsupported input type ");
    std::vector<Input> vec6{};
    CHECK(emitInputs(vec6) == "");
}

TEST_CASE("testing emit of assignment statement.") {
    SUBCASE("emitting full assignment statement.") {
        CHECK(emitAssignmentStmt("foo", "=", "42 * 5") == "foo = 42 * 5;\n");
        CHECK(emitAssignmentStmt("foo", "+=", "0.25") == "foo += 0.25;\n");
        CHECK(emitAssignmentStmt("foo", "-=", "0.25") == "foo -= 0.25;\n");
        CHECK(emitAssignmentStmt("foo", "*=", "0.25") == "foo *= 0.25;\n");
        CHECK(emitAssignmentStmt("foo", "/=", "0.25") == "foo /= 0.25;\n");

        CHECK_THROWS_WITH(emitAssignmentStmt("foo", "14", "0.123"),
                          "14 is not a valid assignment operator.");
    }
    SUBCASE("emitting front of assignment statement.") {
        CHECK(emitAssignmentFront("foo", "=") == "foo =");
        CHECK(emitAssignmentFront("foo", "+=") == "foo +=");
        CHECK(emitAssignmentFront("foo", "-=") == "foo -=");
        CHECK(emitAssignmentFront("foo", "*=") == "foo *=");
        CHECK(emitAssignmentFront("foo", "/=") == "foo /=");

        CHECK_THROWS_WITH(emitAssignmentFront("foo", "14"),
                          "14 is not a valid assignment operator.");
    }
    SUBCASE("emitting back of assignment statement.") {
        CHECK(emitAssignmentBack() == ";\n");
    }
}

TEST_CASE("testing emit of cast expression.") {
    CHECK(emitCastFront("int") == "static_cast<int>(");
    CHECK(emitCastFront("angle") == "static_cast<double>(");
    CHECK(emitCastFront("float") == "static_cast<double>(");
    CHECK(emitCastBack() == ")");
}

TEST_CASE("testing emit of function call statement.") {
    SUBCASE("testing general function call.") {
        CHECK(emitFuncCallStmt("foo", "0.15, \"hello\", 42") ==
              "foo(0.15, \"hello\", 42)");
    }
    SUBCASE("testing built-in function calls.") {
        CHECK(emitFuncCallStmt("arccos", "0.15") == "arccos(0.15)");
        CHECK(emitFuncCallStmt("arcsin", "0.15") == "arcsin(0.15)");
        CHECK(emitFuncCallStmt("arctan", "0.15") == "arctan(0.15)");
        CHECK(emitFuncCallStmt("sin", "0.15") == "sin(0.15)");
        CHECK(emitFuncCallStmt("cos", "0.15") == "cos(0.15)");
        CHECK(emitFuncCallStmt("tan", "0.15") == "tan(0.15)");
        CHECK(emitFuncCallStmt("ceiling", "0.15") == "ceiling(0.15)");
        CHECK(emitFuncCallStmt("floor", "0.15") == "floor(0.15)");
        CHECK(emitFuncCallStmt("exp", "0.15") == "exp(0.15)");
        CHECK(emitFuncCallStmt("log", "0.15") == "log(0.15)");
        CHECK(emitFuncCallStmt("mod", "42, 2") == "mod(42, 2)");
        CHECK(emitFuncCallStmt("sqrt", "0.15") == "sqrt(0.15)");
    }
}

TEST_CASE("testing emit of expression statement.") {
    CHECK(emitExprStmt("a > 3") == "a > 3");
    CHECK(emitExprStmt("a > 3 * 42") == "a > 3 * 42");
}

TEST_CASE("testing emit of measurement.") {
    CHECK(emitMeasurement("qubits", "3") ==
          "applyQubitMeasurement(qubits, 3);\n");
}

TEST_CASE("testing emit of arthmetic operators.") {
    CHECK(emitPlusOp() == " + ");
    CHECK(emitMinusOp() == " - ");
    CHECK(emitMultOp() == " * ");
    CHECK(emitDivOp() == " / ");
    CHECK(emitModOp() == " % ");
    CHECK(emitPowOp("3", "2") == "pow(3, 2)");
}

TEST_CASE("testing emit of logical operators.") {
    CHECK(emitDoubleAmpersandOp() == " && ");
    CHECK(emitDoublePipeOp() == " || ");
    CHECK(emitEqualityOp("==") == " == ");
    CHECK(emitEqualityOp("!=") == " != ");
    CHECK_THROWS_WITH(emitEqualityOp("12"), "Invalid equality operator 12");
    CHECK(emitCompOp("<") == " < ");
    CHECK(emitCompOp(">") == " > ");
    CHECK_THROWS_WITH(emitCompOp("12"), "Invalid comparison operator 12");
}

TEST_CASE("testing emit of bit operators.") {
    CHECK(emitAmpersandOp() == " & ");
    CHECK(emitPipeOp() == " | ");
    CHECK(emitXorOp() == " ^ ");
    CHECK(emitBitshiftOp("<<") == " << ");
    CHECK(emitBitshiftOp(">>") == " >> ");
    CHECK_THROWS_WITH(emitBitshiftOp("12"), "Invalid bitshift operator 12");
}

TEST_CASE("testing unary operators.") {
    CHECK(emitMinusUnary() == "-");
    CHECK(emitExclamationUnary() == "!");
    CHECK(emitTildeUnary() == "~");
}

TEST_CASE("testing emit of literal.") {
    CHECK(emitLiteral("4") == "4");
    CHECK(emitLiteral("ahskfh") == "ahskfh");
}

TEST_CASE("testing emit of scalar type.") {
    CHECK(emitScalarType("int[16]") == "int");
    CHECK(emitScalarType("double[16]") == "double");
    CHECK(emitScalarType("angle[32]") == "double");
    CHECK(emitScalarType("float[32]") == "double");
    CHECK(emitScalarType("float") == "double");
    CHECK(emitScalarType("angle") == "double");
    CHECK(emitScalarType("int") == "int");
}

TEST_CASE("testing emit of ndarray type.") {
    CHECK(emitNdArray(3, "int") ==
          "std::vector<std::vector<std::vector<int>>>");
    CHECK(emitNdArray(2, "int") == "std::vector<std::vector<int>>");
    CHECK(emitNdArray(1, "int") == "std::vector<int>");
    CHECK(emitNdArray(0, "int") == "int");
    CHECK_THROWS_WITH(
        emitNdArray(-1, "int"),
        "error in emitNdArray, passed negative number of array dims: -1.");
}
