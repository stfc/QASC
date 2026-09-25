/**
 * @copyright Copyright (c) 2025-2026
 * UK Research and Innovation,
 * Science and Technology Facilities Council,
 * Hartree Centre
 **/

#include "config_paths.h"
#include "qasm3Lexer.h"
#include "qasm3Parser.h"
#include "src/collectErrorStrategy.h"
#include "src/compiler.h"

#include <format>
#include <string>
#include <string_view>
#include "doctest.h"

namespace {
std::string compileText(std::string_view str) {
    antlr4::ANTLRInputStream input{ str };
    qasm3Lexer lexer{ &input };
    antlr4::CommonTokenStream tokens(&lexer);
    qasm3Parser parser{ &tokens };
    auto error_strat = std::make_shared<CollectErrorStrategy>();
    parser.setErrorHandler(error_strat);
    qasm3Parser::ProgramContext* tree = nullptr;
    tree = parser.program();
    CollectErrorStrategy* errorStrat
        = dynamic_cast<CollectErrorStrategy*>(parser.getErrorHandler().get());
    errorStrat->reportErrors();

    Compiler compiler{};
    compiler.visitProgram(tree);
    return compiler.getCode();
}

std::string parsingErrorMsg(int numErrors) {
    return std::format("Encountered {} errors while parsing.", numErrors);
}

std::string completeCode(std::string_view code,
                         std::string_view codeBeforeMain = "",
                         bool withMainFn = true) {
    const std::string headers{
        "#include \"quest.h\"\n#include \"argparse.hpp\"\n#include "
        "<vector>\n#include <type_traits>\n#include <fstream>\n#include "
        "<iostream>\n#include <cmath>\n#include <algorithm>\n#include "
        "\"qasc_gates.h\"\n\n"

    };
    if (withMainFn) {
        const auto qascNamespace{ "using namespace qasc;\n" };
        const auto mainFuncFront{ "int main(int argc, char* argv[]) {\n" };
        const auto mainFuncBack{ "\nreturn 0;\n}\n" };
        return std::format("{}{}{}{}{}{}", headers, qascNamespace,
                           codeBeforeMain, mainFuncFront, code, mainFuncBack);
    } else {
        return std::format("{}{}", headers, code);
    }
}

std::string questCode(std::string_view numQubits) {
    return "if (!isQuESTEnvInit()) {\n"
           "initQuESTEnv();\n"
           "}\n"
           "if (!isQascInternalQuestQuregInit) {\n"
           "isQascInternalQuestQuregInit = true;\n"
           + std::format("qascInternalQuestQureg = createQureg({});\n",
                         numQubits)
           + "}\n";
}

std::string endQuestCode() {
    return "finalizeQuESTEnv();\n";
}

}    // namespace

TEST_CASE("testing annotation directive.") {
    CHECK_THROWS_WITH(
        compileText("@bind IOPORT[3:2]\ninput int foo;\n"),
        "<noname> \033[1;31merror: \033[0mannotation directives are not "
        "supported.\n1 | @bind IOPORT[3:2]\n");
}

TEST_CASE("testing pragma directive.") {
    CHECK_THROWS_WITH(
        compileText("pragma something.x noise 0.85"),
        "<noname> \033[1;31merror: \033[0mpragma directives are not "
        "supported.\n1 | pragma something.x noise 0.85\n");
}

TEST_CASE("testing calibration grammar statement.") {
    CHECK_THROWS_WITH(compileText("defcalgrammar \"mygrammar\";"),
                      "<noname> \033[1;31merror: \033[0mcalibration grammar "
                      "statements are not supported.\n1 | defcalgrammar "
                      "\"mygrammar\";\n");
}

TEST_CASE("testing box statement.") {
    CHECK(compileText("box {cx a, b;}")
          == completeCode("{\n" + questCode("1") + "cx(a,b);\n}\n"
                          + endQuestCode()));
}

TEST_CASE("testing parsing of empty expression.") {
    CHECK(compileText("") == completeCode(""));
}

TEST_CASE("testing end statement.") {
    CHECK(compileText("end;") == completeCode("exit(0);\n"));
}

TEST_CASE("testing parsing for loop statement.") {
    SUBCASE("for loop with missing iterator fails.") {
        CHECK_THROWS_WITH(compileText("for in [0:10] { x+=1;}"),
                          parsingErrorMsg(3).c_str());
    }

    SUBCASE("for loop with missing identifier fails.") {
        CHECK_THROWS_WITH(compileText("for int in [0:10] { x+=1;}"),
                          parsingErrorMsg(1).c_str());
    }

    SUBCASE("correct for loop generates good code.") {
        CHECK(compileText("for int i in [0:foo] { x+=1;}")
              == completeCode(
                  "for (auto i = 0; i < foo  + 1; i += 1){\nx +=1;\n}\n"));

        CHECK(compileText("for int i in [0:2:22] { x+=1;}")
              == completeCode("for (auto i = 0; i < 22  + 1; i += 2){\nx "
                              "+=1;\n}\n"));

        int x = 0;
        for (auto i = 0; i < 22 + 1; i += 2) {
            x += i;
        }
        CHECK(x == 132);
    }

    SUBCASE("correct for loop generates good code.") {
        CHECK(compileText("for int i in foo { x+=1;}")
              == completeCode("for (auto& i : foo){\nx +=1;\n}\n"));
    }

    SUBCASE("for loop with empty set expression fails.") {
        CHECK_THROWS_WITH(compileText("for int i in {} { x+=1;}"),
                          parsingErrorMsg(1).c_str());
    }

    SUBCASE("for loop with valid set expression.") {
        CHECK(compileText("for int i in {0, foo, 2, 3} { x+=1;}")
              == completeCode(
                  "for (auto v = std::vector{0,foo,2,3}; auto& i : v){\nx "
                  "+=1;\n}\n"));

        CHECK(compileText("for int[32] i in {2, 3, 5} { x+=1;}")
              == completeCode(
                  "for (auto v = std::vector{2,3,5}; auto& i : v){\nx "
                  "+=1;\n}\n"));
        int x = 0;
        for (auto v = std::vector{ 2, 3, 5 }; auto& i : v) {
            x += i;
        }
        CHECK(x == 10);
    }

    SUBCASE("for loop with array of floats") {
        CHECK(compileText(
                  "array[float[64], 5] arr = {0.5, 1.5, -1.5, 2.0, 3.3};\n"
                  "for float[64] f in arr {\n"
                  "x += f;\n"
                  "}\n")
              == completeCode(
                  "for (auto& f : arr){\n"
                  "x +=f;\n"
                  "}\n",
                  "std::vector<double> arr = {0.5,1.5,-1.5,2.0,3.3};\n"));

        std::vector<double> arr = { 0.5, 1.5, -1.5, 2.0, 3.3 };
        double x = 0.0;
        for (auto& f : arr) {
            x += f;
        }
        CHECK(x - 5.8 < 1e-9);
    }

    SUBCASE("for loop with classical register (of bits)") {
        CHECK(compileText("bit[6] reg = \"010101\";\n"
                          "for bit b in reg {}")
              == completeCode("for (auto& b : reg){\n}\n",
                              "QasmType<unsigned int, 6> reg(\"010101\");\n"));
    }

    SUBCASE("for loop with no set and range expressions fails.") {
        CHECK_THROWS_WITH(compileText("for int i in { x+=1;}"),
                          parsingErrorMsg(2).c_str());
    }

    SUBCASE("for loop with no scope fails.") {
        CHECK_THROWS_WITH(compileText("for int i in "),
                          parsingErrorMsg(1).c_str());
    }
}

TEST_CASE("testing parsing if-else statement.") {
    SUBCASE("if statement without condition fails.") {
        CHECK_THROWS_WITH(compileText("if () {x+=1;} "),
                          parsingErrorMsg(1).c_str());
    }

    SUBCASE("if statement with no body fails.") {
        CHECK_THROWS_WITH(compileText("if (i < 10)"),
                          parsingErrorMsg(1).c_str());
    }

    SUBCASE("if-else statement with no if body fails.") {
        CHECK_THROWS_WITH(compileText("if (i < 10) else {x+=1;}"),
                          parsingErrorMsg(1).c_str());
    }

    SUBCASE("correct if statement generates good code.") {
        CHECK(compileText("if (i < 10) {x+=1;}")
              == completeCode("if (i<10)\n{\nx +=1;\n}\n"));
    }

    SUBCASE("correct if-else statement generates good code.") {
        CHECK(compileText("if (i < 10) {x+=1;} else {x+=2;}")
              == completeCode("if (i<10)\n{\nx +=1;\n}\nelse\n{\nx +=2;\n}\n"));
    }

    SUBCASE(
        "if statements with more complex conditionals generate good code.") {
        CHECK(compileText("if (2 ** 2 < 5) {x+=1;}")
              == completeCode("if (pow(2, 2)<5)\n{\nx +=1;\n}\n"));
    }

    SUBCASE("if statement with one line body work without curly braces.") {
        CHECK(compileText("qubit q;\n"
                          "if (basis == 0) h q;\n"
                          "else if (basis == 1) rx(pi/2) q;\n"
                          "result = measure q;\n")
              == completeCode("auto q = QuregSlice(1);\n" + questCode("0 + 1")
                              + "if (basis==0)\n"
                                "h(q);\n"
                                "else\n"
                                "if (basis==1)\n"
                                "rx(3.14159265358979323846/2, q);\n"
                                "result =measure(q);\n"
                              + endQuestCode()));
    }
}

TEST_CASE("testing parsing return statement.") {
    CHECK(compileText("return 1;") == completeCode("return 1;\n"));
    CHECK(compileText("return;") == completeCode("return ;\n"));
}

TEST_CASE("testing parsing while statement.") {
    SUBCASE("while statement with missing condition fails.") {
        CHECK_THROWS_WITH(compileText("while () {x += 1;}"),
                          parsingErrorMsg(1).c_str());
    }

    SUBCASE("while statement with missing scope fails.") {
        CHECK_THROWS_WITH(compileText("while (i < 10)"),
                          parsingErrorMsg(1).c_str());
    }

    SUBCASE("correct while statement produces good code.") {
        CHECK(compileText("while (i < 10) {x += 1;}")
              == completeCode("while (i<10)\n{\nx +=1;\n}\n"));
    }

    SUBCASE("while loop with quantum ops produces good code.") {
        CHECK(compileText("qubit q;\n"
                          "int x = 0;"
                          "while (i < 10) {\n"
                          "h q;\n"
                          "bit res = measure q;\n"
                          "if (res) {\n"
                          "x += 1;\n"
                          "}\n"
                          "}")
              ==

              completeCode("auto q = QuregSlice(1);\n" + questCode("0 + 1")
                           + "QasmType<int, 32> x(0);\n"
                             "while (i<10)\n{\n"
                             "h(q);\n"
                             "QasmType<unsigned int, 1> res(measure(q));\n"
                             "if (res)\n{\n"
                             "x +=1;\n"
                             "}\n"
                             "}\n"
                           + endQuestCode()));
    }

    SUBCASE("complex while loop produces good code.") {
        CHECK(compileText("int x = 0;"
                          "while (x < 11) {\n"
                          "x += 1;\n"
                          "if (x == 3) {continue;}\n"
                          "if (x == 5) {break;}\n"
                          "x += 11;\n"
                          "}")
              == completeCode("while (x<11)\n{\n"
                              "x +=1;\n"
                              "if (x==3)\n{\n"
                              "continue;\n"
                              "}\n"
                              "if (x==5)\n{\n"
                              "break;\n"
                              "}\n"
                              "x +=11;\n"
                              "}\n",
                              "QasmType<int, 32> x(0);\n"));
    }
}

TEST_CASE("testing parsing switch-case statement.") {
    SUBCASE("switch statement with missing condition fails.") {
        CHECK_THROWS_WITH(compileText("switch () {case 1 {x += 2;}}"),
                          parsingErrorMsg(1).c_str());
    }

    SUBCASE("correct switch statement generates good code.") {
        CHECK(compileText("switch (foo) {case 1 {x += 2;}}")
              == completeCode(
                  "switch (foo)\n{\ncase 1: {\n{\nx +=2;\n}\nbreak;\n}\n}\n"));
        CHECK(
            compileText(
                "switch (foo) {case 1 {x += 2;} case 2 {x += 1;} default { x = "
                "3;}}")
            == completeCode(
                "switch (foo)\n{\ncase 1: {\n{\nx +=2;\n}\nbreak;\n}\ncase 2: "
                "{\n{\nx +=1;\n}\nbreak;\n}\ndefault: {\n{\nx "
                "=3;\n}\nbreak;\n}\n}\n"));
    }

    SUBCASE("switch with multiple cases per case-body generates good code.") {
        CHECK(compileText("int opt = 42;\n"
                          "switch (opt) {\n"
                          "case 4, 2, 6 {\n"
                          "opt += 5;\n"
                          "}\n"
                          "case -4, -2, -6 {\n"
                          "opt -= 5;\n"
                          "}\n"
                          "case 9, 11, 42 {\n"
                          "opt += 9;\n"
                          "}\n"
                          "default {\n"
                          "opt -= 42;\n"
                          "}\n"
                          "}\n")
              == completeCode(""
                              "switch (opt)\n"
                              "{\n"
                              "case 4: {\n"
                              "{\n"
                              "opt +=5;\n"
                              "}\n"
                              "break;\n"
                              "}\n"
                              "case 2: {\n"
                              "{\n"
                              "opt +=5;\n"
                              "}\n"
                              "break;\n"
                              "}\n"
                              "case 6: {\n"
                              "{\n"
                              "opt +=5;\n"
                              "}\n"
                              "break;\n"
                              "}\n"
                              "case -4: {\n"
                              "{\n"
                              "opt -=5;\n"
                              "}\n"
                              "break;\n"
                              "}\n"
                              "case -2: {\n"
                              "{\n"
                              "opt -=5;\n"
                              "}\n"
                              "break;\n"
                              "}\n"
                              "case -6: {\n"
                              "{\n"
                              "opt -=5;\n"
                              "}\n"
                              "break;\n"
                              "}\n"
                              "case 9: {\n"
                              "{\n"
                              "opt +=9;\n"
                              "}\n"
                              "break;\n"
                              "}\n"
                              "case 11: {\n"
                              "{\n"
                              "opt +=9;\n"
                              "}\n"
                              "break;\n"
                              "}\n"
                              "case 42: {\n"
                              "{\n"
                              "opt +=9;\n"
                              "}\n"
                              "break;\n"
                              "}\n"
                              "default: {\n"
                              "{\n"
                              "opt -=42;\n"
                              "}\n"
                              "break;\n"
                              "}\n"
                              "}\n",
                              "QasmType<int, 32> opt(42);\n"));
    }
    SUBCASE("switch with cases as variables generates good code.") {
        CHECK(compileText("const int A = -3;\n"
                          "const int B = 21;\n"
                          "int i = 19;\n"
                          "\n"
                          "switch (i) {\n"
                          "case A {\n"
                          "i += 3;\n"
                          "}\n"
                          "case B-2 {\n"
                          "i += 4;\n"
                          "}\n"
                          "case B+3 {\n"
                          "i += 5;\n"
                          "}\n"
                          "default {\n"
                          "i += 6;\n"
                          "}\n"
                          "}\n")
              == completeCode("switch (i)\n"
                              "{\n"
                              "case A: {\n"
                              "{\n"
                              "i +=3;\n"
                              "}\n"
                              "break;\n"
                              "}\n"
                              "case B-2: {\n"
                              "{\n"
                              "i +=4;\n"
                              "}\n"
                              "break;\n"
                              "}\n"
                              "case B+3: {\n"
                              "{\n"
                              "i +=5;\n"
                              "}\n"
                              "break;\n"
                              "}\n"
                              "default: {\n"
                              "{\n"
                              "i +=6;\n"
                              "}\n"
                              "break;\n"
                              "}\n"
                              "}\n",
                              "constexpr QasmType<int, 32> A(-3);\n"
                              "constexpr QasmType<int, 32> B(21);\n"
                              "QasmType<int, 32> i(19);\n"));
    }

    SUBCASE("switch on binary casted to int generates good code.") {
        CHECK(compileText("int res = 0;\n"
                          "bit[2] i;\n"
                          "\n"
                          "switch (int(i)) {\n"
                          "case 0b00 {\n"
                          "res += 3;\n"
                          "}\n"
                          "case 0b01 {\n"
                          "res += 4;\n"
                          "}\n"
                          "case 0b10 {\n"
                          "res += 5;\n"
                          "}\n"
                          "case 0b11 {\n"
                          "res += 6;\n"
                          "}\n"
                          "default {\n"
                          "res += 7;\n"
                          "}\n"
                          "}\n")
              == completeCode("switch (static_cast<QasmType<int>>(i))\n"
                              "{\n"
                              "case 0b00: {\n"
                              "{\n"
                              "res +=3;\n"
                              "}\n"
                              "break;\n"
                              "}\n"
                              "case 0b01: {\n"
                              "{\n"
                              "res +=4;\n"
                              "}\n"
                              "break;\n"
                              "}\n"
                              "case 0b10: {\n"
                              "{\n"
                              "res +=5;\n"
                              "}\n"
                              "break;\n"
                              "}\n"
                              "case 0b11: {\n"
                              "{\n"
                              "res +=6;\n"
                              "}\n"
                              "break;\n"
                              "}\n"
                              "default: {\n"
                              "{\n"
                              "res +=7;\n"
                              "}\n"
                              "break;\n"
                              "}\n"
                              "}\n",
                              "QasmType<int, 32> res(0);\n"
                              "QasmType<unsigned int, 2> i(0);\n"));
    }
    SUBCASE(
        "switch with complex cases calling quantum functions generates good "
        "code.") {
        CHECK(compileText("def foo(int i, qubit[8] d) -> bit {\n"
                          "  return measure d[i];\n"
                          "}\n"
                          "\n"
                          "int i = 15;\n"
                          "\n"
                          "int j = 1;\n"
                          "int k = 2;\n"
                          "\n"
                          "bit c1;\n"
                          "\n"
                          "qubit[8] q0;\n"
                          "\n"
                          "switch (i) {\n"
                          "        case 1 {\n"
                          "          j = k + foo(k, q0);\n"
                          "        }\n"
                          "        case 2 {\n"
                          "          float[64] d = j / k;\n"
                          "        }\n"
                          "        case 3 {\n"
                          "        }\n"
                          "        default {\n"
                          "        }\n"
                          "}\n")
              == completeCode(
                  "auto q0 = QuregSlice(8);\n" + questCode("0 + 8")
                      + "switch (i)\n"
                        "{\n"
                        "case 1: {\n"
                        "{\n"
                        "j =k+foo(k,q0);\n"
                        "}\n"
                        "break;\n"
                        "}\n"
                        "case 2: {\n"
                        "{\n"
                        "double d = j / k;\n"
                        "}\n"
                        "break;\n"
                        "}\n"
                        "case 3: {\n"
                        "{\n"
                        "}\n"
                        "break;\n"
                        "}\n"
                        "default: {\n"
                        "{\n"
                        "}\n"
                        "break;\n"
                        "}\n"
                        "}\n"
                      + endQuestCode(),
                  "template <QasmQuregT T>\n"
                  "QasmType<unsigned int, 1> foo(QasmType<int, 32> i, T d)\n"
                  "{\n"
                  "return measure(d[i]);\n"
                  "}\n"
                  "QasmType<int, 32> i(15);\n"
                  "QasmType<int, 32> j(1);\n"
                  "QasmType<int, 32> k(2);\n"
                  "QasmType<unsigned int, 1> c1(0);\n"));
    }

    SUBCASE("nested switch generates correct code.") {
        CHECK(compileText("def foo(qubit[8] q) -> int {\n"
                          "  int r = 0;\n"
                          "  bit k;\n"
                          "\n"
                          "  for int i in [0 : 7] {\n"
                          "        k = measure q[i];\n"
                          "        r += k;\n"
                          "  }\n"
                          "\n"
                          "  return r;\n"
                          "}\n"
                          "qubit[8] q;\n"
                          "int j = 30;\n"
                          "int i = foo(q);\n"
                          "switch (i) {\n"
                          "        case 1, 2, 5, 12 { }\n"
                          "        case 3 {\n"
                          "                switch (j) {\n"
                          "                        case 10, 15, 20 {\n"
                          "                                h q;\n"
                          "                        }\n"
                          "                }\n"
                          "        }\n"
                          "}\n"

                          )
              == completeCode("auto q = QuregSlice(8);\n" + questCode("0 + 8")
                                  + "QasmType<int, 32> j(30);\n"
                                    "QasmType<int, 32> i(foo(q));\n"
                                    "switch (i)\n"
                                    "{\n"
                                    "case 1: {\n"
                                    "{\n"
                                    "}\n"
                                    "break;\n"
                                    "}\n"
                                    "case 2: {\n"
                                    "{\n"
                                    "}\n"
                                    "break;\n"
                                    "}\n"
                                    "case 5: {\n"
                                    "{\n"
                                    "}\n"
                                    "break;\n"
                                    "}\n"
                                    "case 12: {\n"
                                    "{\n"
                                    "}\n"
                                    "break;\n"
                                    "}\n"
                                    "case 3: {\n"
                                    "{\n"
                                    "switch (j)\n"
                                    "{\n"
                                    "case 10: {\n"
                                    "{\n"
                                    "h(q);\n"
                                    "}\n"
                                    "break;\n"
                                    "}\n"
                                    "case 15: {\n"
                                    "{\n"
                                    "h(q);\n"
                                    "}\n"
                                    "break;\n"
                                    "}\n"
                                    "case 20: {\n"
                                    "{\n"
                                    "h(q);\n"
                                    "}\n"
                                    "break;\n"
                                    "}\n"
                                    "}\n"
                                    "}\n"
                                    "break;\n"
                                    "}\n"
                                    "}\n"
                                  + endQuestCode(),
                              "template <QasmQuregT T>\n"
                              "QasmType<int, 32> foo(T q)\n"
                              "{\n"
                              "QasmType<int, 32> r(0);\n"
                              "QasmType<unsigned int, 1> k(0);\n"
                              "for (auto i = 0; i < 7  + 1; i += 1){\n"
                              "k =measure(q[i]);\n"
                              "r +=k;\n"
                              "}\n"
                              "return r;\n"
                              "}\n"));
    }
}

TEST_CASE("testing gate call statement.") {
    SUBCASE("single qubit") {
        for (auto v
             = std::vector{ "x", "y", "z", "s", "sdg", "sx", "t", "tdg" };
             auto& e : v) {
            CHECK(compileText(std::format("{} qubits[0];", e))
                  == completeCode(questCode("1")
                                  + std::format("{}(qubits[0]);\n", e)
                                  + endQuestCode()));
            CHECK_THROWS_WITH(
                compileText(std::format("{} qubits[0] qubits[3];", e)),
                parsingErrorMsg(1).c_str());
        }
    }
    SUBCASE("rotation qubit") {
        for (auto v = std::vector{ "rx", "ry", "rz", "p" }; auto& e : v) {
            CHECK(compileText(std::format("{}(3.141) qubits[0];", e))
                  == completeCode(questCode("1")
                                  + std::format("{}(3.141, qubits[0]);\n", e)
                                  + endQuestCode()));
            CHECK_THROWS_WITH(
                compileText(std::format("{} qubits[0] qubits[3];", e)),
                parsingErrorMsg(1).c_str());
        }
    }
    SUBCASE("two qubit") {
        for (auto v = std::vector{ "cx", "cy", "cz", "swap", "ch" };
             auto& e : v) {
            CHECK(compileText(std::format("{} qubits[0], qubits[3];", e))
                  == completeCode(questCode("1")
                                  + std::format("{}(qubits[0],qubits[3]);\n", e)
                                  + endQuestCode()));

            // NOTE: invalid formats will fail to compile at backend stage
        }
    }

    SUBCASE("control rotation qubit") {
        for (auto v = std::vector{ "crx", "cry", "crz", "cp" }; auto& e : v) {
            CHECK(compileText(std::format("{}(3.141) qubits[0], qubits[3];", e))
                  == completeCode(
                      questCode("1")
                      + std::format("{}(3.141, qubits[0],qubits[3]);\n", e)
                      + endQuestCode()));
            CHECK_THROWS_WITH(
                compileText(std::format("{} qubits[0] qubits[3];", e)),
                parsingErrorMsg(1).c_str());
        }
    }

    SUBCASE("three qubit") {
        for (auto v = std::vector{ "cswap", "ccx" }; auto& e : v) {
            CHECK(compileText(
                      std::format("{} qubits[0], qubits[1], qubits[2];", e))
                  == completeCode(
                      questCode("1")
                      + std::format("{}(qubits[0],qubits[1],qubits[2]);\n", e)
                      + endQuestCode()));
        }
    }

    SUBCASE("unknown") {
        CHECK_THROWS_WITH(compileText("foo qubits[0] qubits[3];"),
                          parsingErrorMsg(1).c_str());
    }
    SUBCASE("GPhase") {
        CHECK(compileText("qubit[5] qubits;\ngphase(0.14);\n")
              == completeCode("auto qubits = QuregSlice(5);\n"
                              + questCode("0 + 5") + "gphase(0.14);\n"
                              + endQuestCode()));
    }

    SUBCASE("other QASM gates.") {
        CHECK(compileText("U(0.1, 0.2, 0.3) qubits[0];")
              == completeCode(questCode("1") + "U(0.1,0.2,0.3, qubits[0]);\n"
                              + endQuestCode()));

        CHECK(compileText("cu(0.1, 0.2, 0.3, 0.4) qubits[0], qubits[1];")
              == completeCode(questCode("1")
                              + "cu(0.1,0.2,0.3,0.4, qubits[0],qubits[1]);\n"
                              + endQuestCode()));
    }

    SUBCASE("gate application on qureg slice.") {
        CHECK(compileText("qubit[6] qubits;\nx qubits[1:3];\n")
              == completeCode(
                  "auto qubits = QuregSlice(6);\n" + questCode("0 + 6")
                  + "x(getSlice(qubits, 1, 3, 1));\n" + endQuestCode()));
    }
}

TEST_CASE("testing gate definition statement") {
    CHECK(compileText("qubit[2] q;\n"
                      "gate mygate(a) b, c {\n"
                      "rx(a) b;\n"
                      "h c;\n"
                      "rz(a) c;\n"
                      "h c;\n"
                      "}\n"
                      "mygate(pi) q[0], q[1];\n")
          == completeCode("auto q = QuregSlice(2);\n" + questCode("0 + 2")
                              + "mygate(3.14159265358979323846, q[0],q[1]);\n"
                              + endQuestCode(),
                          "template <QasmQuregT T>\n"
                          "void mygate(double a, T b, T c)\n"
                          "{\n"
                          "rx(a, b);\n"
                          "h(c);\n"
                          "rz(a, c);\n"
                          "h(c);\n"
                          "}\n"));

    CHECK(compileText("qubit[3] q;\n"
                      "gate mygate1(a, b) c, d {\n"
                      "rx(a) c;\n"
                      "ry(b) d;\n"
                      "}\n"
                      "gate mygate2(a, b) c, d {\n"
                      "rx(a) c;\n"
                      "ry(b) d;\n"
                      "}\n"
                      "float a = 0.14;\n"
                      "float b = pi;  \n"
                      "box [150ns] {\n"
                      "delay[str1] q1;\n"
                      "mygate1(a, a+b) q[0], q[1];\n"
                      "mygate2(a, a-b) q[1], q[2];\n"
                      "mygate1(a-b, b) q[0], q[1];\n"
                      "}\n")
          == completeCode("auto q = QuregSlice(3);\n" + questCode("0 + 3")
                              + "double a = 0.14;\n"
                                "double b = 3.14159265358979323846;\n"
                                "{\n"
                                "mygate1(a,a+b, q[0],q[1]);\n"
                                "mygate2(a,a-b, q[1],q[2]);\n"
                                "mygate1(a-b,b, q[0],q[1]);\n"
                                "}\n"
                              + endQuestCode()

                              ,
                          "template <QasmQuregT T>\n"
                          "void mygate1(double a, double b, T c, T d)\n"
                          "{\n"
                          "rx(a, c);\n"
                          "ry(b, d);\n"
                          "}\n"
                          "template <QasmQuregT T>\n"
                          "void mygate2(double a, double b, T c, T d)\n"
                          "{\n"
                          "rx(a, c);\n"
                          "ry(b, d);\n"
                          "}\n"

                          ));

    SUBCASE("general gate definitions.") {
        CHECK(compileText("gate g1 a {}\n"
                          "gate g2 a, b {}\n"
                          "gate g3 a, b, c {}\n"
                          "qubit[1] q1;\n"
                          "qubit[1] q2;\n"
                          "qubit[1] q3;\n"
                          "g1 q1;\n"
                          "g2 q1, q2;\n"
                          "g3 q1, q2, q3;\n")
              == completeCode("auto q1 = QuregSlice(1);\n"
                              "auto q2 = QuregSlice(1);\n"
                              "auto q3 = QuregSlice(1);\n"
                                  + questCode("0 + 1 + 1 + 1")
                                  + "g1(q1);\n"
                                    "g2(q1,q2);\n"
                                    "g3(q1,q2,q3);\n"
                                  + endQuestCode(),
                              "template <QasmQuregT T>\n"
                              "void g1( T a)\n"
                              "{\n"
                              "}\n"
                              "template <QasmQuregT T>\n"
                              "void g2( T a, T b)\n"
                              "{\n"
                              "}\n"
                              "template <QasmQuregT T>\n"
                              "void g3( T a, T b, T c)\n"
                              "{\n"
                              "}\n"

                              ));
    }

    SUBCASE("gate modifiers.") {
        CHECK(compileText("ctrl @ p(0.1) qubits[0], qubits[2];")
              == completeCode(questCode("1") + "cp(0.1, qubits[0],qubits[2]);\n"
                              + endQuestCode()));

        CHECK_THROWS_WITH(compileText("ctrl @ sdg qubits[0], qubits[2];"),
                          "<noname> \033[1;31merror: \033[0mctrl modifier can "
                          "be applied only "
                          "to the following gates: x X y z h s t u U p swap cz "
                          "phase rx ry rz \n1 | ctrl @\n");

        CHECK_THROWS_WITH(compileText("ctrl @ sx qubits[0], qubits[2];"),
                          "<noname> \033[1;31merror: \033[0mctrl modifier can "
                          "be applied only "
                          "to the following gates: x X y z h s t u U p swap cz "
                          "phase rx ry rz \n1 | ctrl @\n");

        CHECK_THROWS_WITH(compileText("ctrl @ tdg qubits[0], qubits[2];"),
                          "<noname> \033[1;31merror: \033[0mctrl modifier can "
                          "be applied only "
                          "to the following gates: x X y z h s t u U p swap cz "
                          "phase rx ry rz \n1 | ctrl @\n");

        CHECK(compileText("ctrl @ U(0.1, 0.2, 0.3) qubits[0], qubits[2];")
              == completeCode(questCode("1")
                              + "cU(0.1,0.2,0.3, qubits[0],qubits[2]);\n"
                              + endQuestCode()));

        CHECK_THROWS_WITH(
            compileText("ctrl(2) @ cx qubits[0], qubits[1];"),
            "<noname> \033[1;31merror: \033[0mmulti-qubit CTRL modifier is "
            "unsupported. Also "
            "change ctrl(1) @ -> ctrl @\n1 | ctrl(2) @\n");

        CHECK(compileText("pow(3) @ x qubits[0];")
              == completeCode(questCode("1")
                              + "for (auto gatePowerIdx = 0; gatePowerIdx < 3; "
                                "++gatePowerIdx) {\n"
                                "x(qubits[0]);\n\n"
                                "}\n"
                              + endQuestCode()));

        CHECK(compileText("inv @ x qubits[0];")
              == completeCode(questCode("1") + "inv_x(qubits[0]);\n"
                              + endQuestCode()));

        CHECK(compileText("negctrl @ cx qubits[0];")
              == completeCode(questCode("1") + "x(qubits[0]);\n"
                              + endQuestCode()));

        CHECK_THROWS_WITH(
            compileText("negctrl(2) @ ctrl @ cx qubits[0], qubits[1];"),
            "<noname> \033[1;31merror: \033[0mmulti-qubit NEGCTRL modifier is "
            "unsupported. Also "
            "change negctrl(1) @ -> negctrl @\n1 | negctrl(2) @\n");
    }

    SUBCASE("generating QASM2.0 U-rotation gates.") {
        CHECK(compileText("u1(0.14) qubits[0];")
              == completeCode(questCode("1") + "u1(0.14, qubits[0]);\n"
                              + endQuestCode()));

        CHECK(compileText("u2(0.14, 0.13) qubits[0];")
              == completeCode(questCode("1") + "u2(0.14,0.13, qubits[0]);\n"
                              + endQuestCode()));

        CHECK(compileText("u3(0.1, 0.2, 0.3) qubits[0];")
              == completeCode(questCode("1") + "u3(0.1,0.2,0.3, qubits[0]);\n"
                              + endQuestCode()));
    }
}

TEST_CASE("testing measure arrow assignment statement.") {
    CHECK(compileText("measure q[0] -> c[0];")
          == completeCode(questCode("1") + "c[0] =one_qubit_measure(q[0]);\n"
                          + endQuestCode()));

    CHECK(compileText("measure q[0:3] -> c;")
          == completeCode(questCode("1") + "c =measure(getSlice(q, 0, 3, 1));\n"
                          + endQuestCode()));

    CHECK(compileText("measure q[0:2:4] -> c;")
          == completeCode(questCode("1") + "c =measure(getSlice(q, 0, 4, 2));\n"
                          + endQuestCode()));

    CHECK(compileText("measure q -> c[0:3];")
          == completeCode(questCode("1") + "setSlice(c, measure(q), 0, 3, 1);\n"
                          + endQuestCode()));

    CHECK(compileText("measure q -> c[0:3:9];")
          == completeCode(questCode("1") + "setSlice(c, measure(q), 0, 9, 3);\n"
                          + endQuestCode()));

    CHECK(compileText("measure q[0:3] -> c[1:4];")
          == completeCode(
              questCode("1")
              + "setSlice(c, measure(getSlice(q, 0, 3, 1)), 1, 4, 1);\n"
              + endQuestCode()));
}

TEST_CASE("testing reset statement.") {
    CHECK_THROWS_WITH(compileText("reset;"), parsingErrorMsg(1).c_str());
    CHECK(
        compileText("reset qubits;")
        == completeCode(questCode("1") + "reset(qubits);\n" + endQuestCode()));
    CHECK(compileText("reset qubits[2];")
          == completeCode(questCode("1") + "reset(qubits[2]);\n"
                          + endQuestCode()));
}

TEST_CASE("testing aliasing statement.") {
    CHECK(compileText("let foo = bar;")
          == completeCode("", "auto foo = bar;\n"));

    CHECK(compileText("let first = qubits[0];")
          == completeCode("", "auto first = QuregSlice(qubits, 0, 0, 1);\n"));

    CHECK(compileText("let last = qubits[-1];")
          == completeCode("", "auto last = QuregSlice(qubits, -1, -1, 1);\n"));

    CHECK_THROWS_WITH(
        compileText("let selection = qubits[{0, 3, 5}];"),
        "<noname> \033[1;31merror: \033[0marray indexing with sets is "
        "not supported.\n1 | [{0, 3, 5}]\n");

    CHECK(compileText("let sliced = qubits[0:8];")
          == completeCode("", "auto sliced = QuregSlice(qubits, 0, 8, 1);\n"));

    CHECK(compileText("let even = qubits[0:2:8];")
          == completeCode("", "auto even = QuregSlice(qubits, 0, 8, 2);\n"));

    CHECK(compileText("let few_last = qubits[-5:-1];")
          == completeCode("",
                          "auto few_last = QuregSlice(qubits, -5, -1, 1);\n"));

    CHECK_THROWS_WITH(compileText("let foo = bar ++ zar;"),
                      "<noname> \033[1;31merror: \033[0mconcatenated alias "
                      "expressions are not "
                      "supported.\n1 | let foo = bar ++ zar;\n");
}

TEST_CASE("testing classical declaration statement.") {
    CHECK(compileText("int foo;")
          == completeCode("", "QasmType<int, 32> foo(0);\n"));
    CHECK(compileText("int[24] foo;")
          == completeCode("", "QasmType<int, 24> foo(0);\n"));
    CHECK(compileText("int[24] foo = 15;")
          == completeCode("", "QasmType<int, 24> foo(15);\n"));
    CHECK(compileText("float foo;") == completeCode("", "double foo;\n"));
    CHECK(compileText("float[24] foo;") == completeCode("", "double foo;\n"));
    CHECK(compileText("float[24] foo = 0.32;")
          == completeCode("", "double foo = 0.32;\n"));

    CHECK(compileText("angle foo;")
          == completeCode("", "QasmType<unsigned int, 32, true> foo(0);\n"));
    CHECK(compileText("angle [24] foo;")
          == completeCode("", "QasmType<unsigned int, 24, true> foo(0);\n"));
    CHECK(compileText("angle [24] foo = 0.32;")
          == completeCode("", "QasmType<unsigned int, 24, true> foo(0.32);\n"));

    // arrays
    CHECK(
        compileText("array[angle, 2] foo;")
        == completeCode(
            "", "std::vector<QasmType<unsigned int, 32, true>> foo(2, 0);\n"));
    CHECK(
        compileText("array[angle [24], 2] foo;")
        == completeCode(
            "", "std::vector<QasmType<unsigned int, 24, true>> foo(2, 0);\n"));

    CHECK(compileText("array[angle [24], 2] foo = {0.32, 0.12};")
          == completeCode("",
                          "std::vector<QasmType<unsigned int, 24, true>> foo = "
                          "{0.32,0.12};\n"));

    CHECK(compileText(
              "array[int[32], 5, 2] foo = {{0, 1, 2, 3, 4}, {5, 6, 7, 8, 9}};")
          == completeCode("",
                          "std::vector<std::vector<QasmType<int, 32>>> foo = "
                          "{{0,1,2,3,4},{5,6,7,8,9}};\n"));

    // bits
    CHECK(compileText("bit[8] b = \"0010_1010\";")
          == completeCode("", "QasmType<unsigned int, 8> b(\"0010_1010\");\n"));

    CHECK(compileText("bit[8] b = 0b0010_1010;")
          == completeCode("", "QasmType<unsigned int, 8> b(0b00101010);\n"));
}

TEST_CASE("classical functions definitions.") {
    CHECK(compileText("def fn(int par) -> bit {return 0;}\n")
          == completeCode("",
                          "QasmType<unsigned int, 1> fn(QasmType<int, 32> "
                          "par)\n{\nreturn 0;\n}\n"));
    CHECK(
        compileText("def xmeasure(qubit q) -> bit { h q; return measure q; }\n")
        == completeCode("",
                        "template <QasmQuregT T>\n"
                        "QasmType<unsigned int, 1> xmeasure(T "
                        "q)\n{\nh(q);\nreturn measure(q);\n}\n"));

    CHECK(compileText("def phaseMeasure(angle[32] theta, qubit q) -> bit { "
                      "rz(theta) q; h "
                      "q; return measure q; }\n")
          == completeCode(
              "",
              "template <QasmQuregT T>\n"
              "QasmType<unsigned int, 1> "
              "phaseMeasure(QasmType<unsigned int, 32, true> theta, T "
              "q)\n{\n"
              "rz(theta, q);\n"
              "h(q);\n"
              "return measure(q);\n}\n"));

    CHECK(compileText("def xCheck(qubit[4] a, qubit b) -> bit { "
                      "reset b;\n"
                      "for int i in [0: 3] cx a[i], b;\n"
                      "return measure b; }\n")
          == completeCode("",
                          "template <QasmQuregT T>\n"
                          "QasmType<unsigned int, 1> "
                          "xCheck(T a, T b)\n{\n"
                          "reset(b);\n"
                          "for (auto i = 0; i < 3  + 1; i += 1)"
                          "cx(a[i],b);\n"
                          "return measure(b);\n}\n"));

    CHECK(compileText("def parity(bit[n] aInput) -> bit { "
                      "bit a;\n"
                      "for int i in [0: n - 1] a ^= aInput[i];\n"
                      "return a; }\n")
          == completeCode("",
                          "QasmType<unsigned int, 1> "
                          "parity(QasmType<unsigned int, n> aInput)\n{\n"
                          "QasmType<unsigned int, 1> a(0);\n"
                          "for (auto i = 0; i < n-1  + 1; i += 1)"
                          "a ^=aInput[i];\n"
                          "return a;\n}\n"));

    CHECK(compileText("def parity(bit[n] aInput) -> bit[m] { "
                      "bit[m] a;\n"
                      "for int i in [0: n - 1] a ^= aInput[i];\n"
                      "return a; }\n")
          == completeCode("",
                          "QasmType<unsigned int, m> "
                          "parity(QasmType<unsigned int, n> aInput)\n{\n"
                          "QasmType<unsigned int, m> a(0);\n"
                          "for (auto i = 0; i < n-1  + 1; i += 1)"
                          "a ^=aInput[i];\n"
                          "return a;\n}\n"));

    CHECK(
        compileText("def onDetailedArray(readonly array[int[16], 3, 12] arr) {}"
                    "def dynamicArray(readonly array[int[16], #dim = 1] arr) {}"
                    "def mutableArrayRef(mutable array[int[16], #dim = 1] arr) "
                    "{ arr[2] = 42;}"
                    "array[int[16], 6] a;\n"
                    "array[int[16], 5, 8] b;\n"
                    "dynamicArray(a);\n"
                    "dynamicArray(b[1, 0:4]);\n"
                    "mutableArrayRef(a[1:3]);\n")
        == completeCode("dynamicArray(a);\n"
                        "dynamicArray(getSlice(b[1], 0, 4, 1));\n"
                        "mutableArrayRef(getSlice(a, 1, 3, 1));\n",

                        "template <typename VecT0 >\n"
                        "void onDetailedArray(VecT0 && arr)\n"
                        "{\n"
                        "}\n"
                        "template <typename VecT0 >\n"
                        "void dynamicArray(VecT0 && arr)\n"
                        "{\n"
                        "}\n"
                        "template <typename VecT0 >\n"
                        "void mutableArrayRef(VecT0 && arr)\n"
                        "{\n"
                        "arr[2] =42;\n"
                        "}\n"
                        "std::vector<QasmType<int, 16>> a(6, 0);\n"
                        "std::vector<std::vector<QasmType<int, 16>>> b(5, "
                        "std::vector<QasmType<int, 16>>(8, 0));\n"));

    CHECK(compileText(
              "def complexFuncOnArr(readonly array[int[16], #dim = 2] arr) -> "
              "int[32] {\n"
              "uint[32] firstDim = sizeof(arr, 0);\n"
              "uint[32] sndDim = sizeof(arr, 1);\n"
              "int[32] sum = 0;\n"
              "for int i in [0:firstDim - 1] {\n"
              "for int j in [0:sndDim - 1] {\n"
              "sum += int[32](arr[i][j]);\n"
              "}\n"
              "}\n"
              "return sum;\n"
              "}\n"
              "array[int[16], 2, 5] a = {{1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}};\n"
              "int res = complexFuncOnArr(a);\n")
          == completeCode(
              "",
              "template <typename VecT0 >\n"
              "QasmType<int, 32> complexFuncOnArr(VecT0 && arr)\n"
              "{\n"
              "QasmType<unsigned int, 32> firstDim(qasmSizeof(arr,0));\n"
              "QasmType<unsigned int, 32> sndDim(qasmSizeof(arr,1));\n"
              "QasmType<int, 32> sum(0);\n"
              "for (auto i = 0; i < firstDim-1  + 1; i += 1){\n"
              "for (auto j = 0; j < sndDim-1  + 1; j += 1){\n"
              "sum +=static_cast<QasmType<int, 32>>(arr[i][j]);\n"
              "}\n"
              "}\n"
              "return sum;\n"
              "}\n"
              "std::vector<std::vector<QasmType<int, 16>>> a = "
              "{{1,2,3,4,5},{6,7,8,9,10}};\n"
              "QasmType<int, 32> res(complexFuncOnArr(a));\n"

              ));
}

TEST_CASE("testing IO declaration statement.") {
    CHECK(compileText("output int results;")
          == completeCode("", "QasmType<int, 32> results;\n"));

    CHECK(
        compileText("output array[int, 5] results;")
        == completeCode("", "std::vector<QasmType<int, 32>> results(5, 0);\n"));

    CHECK(compileText("output angle results;")
          == completeCode("", "QasmType<unsigned int, 32, true> results;\n"));

    CHECK(
        compileText("output array[angle, 5] results;")
        == completeCode(
            "",
            "std::vector<QasmType<unsigned int, 32, true>> results(5, 0);\n"));

    CHECK(
        compileText("output array[angle, 5, 4] results;")
        == completeCode("",
                        "std::vector<std::vector<QasmType<unsigned int, 32, "
                        "true>>> results(5, std::vector<QasmType<unsigned int, "
                        "32, true>>(4, 0));\n"));

    CHECK(compileText("output float results;")
          == completeCode("", "double results;\n"));

    CHECK(compileText("output array[float, 5] results;")
          == completeCode("", "std::vector<double> results(5, 0);\n"));

    CHECK(compileText("input int results;")
          == completeCode(
              "argparse::ArgumentParser program(\"\");\nprogram.add_argument(\""
              "--results\")\n.required()\n.nargs(argparse::nargs_pattern::at_"
              "least_one)\n."
              "scan<'i', int>()\n.help(\"qasm input value of type: "
              "int\");\ntry\n{\nprogram.parse_args(argc, "
              "argv);\n}\ncatch "
              "(const "
              "std::exception& err)\n{\nstd::cerr << err.what() << "
              "std::endl;\nstd::cerr << program;\nreturn -1;\n}\n"
              "QasmType<int, 32> results = program.get<QasmType<int, "
              "32>>(\"results\");\n"));

    CHECK(compileText("input array[int, 4] results;")
          == completeCode(
              "argparse::ArgumentParser program(\"\");\nprogram.add_argument(\""
              "--results\")\n.required()\n.nargs(argparse::nargs_pattern::at_"
              "least_one)\n."
              "scan<'i', int>()\n.help(\"qasm input value of type: "
              "array[int]\");\ntry\n{\nprogram.parse_args(argc, "
              "argv);\n}\ncatch "
              "(const "
              "std::exception& err)\n{\nstd::cerr << err.what() << "
              "std::endl;\nstd::cerr << program;\nreturn -1;\n}\n"
              "std::vector<QasmType<int, 32>> results = "
              "program.get<std::vector<QasmType<int, "
              "32>>>(\"results\");\n"));

    CHECK(compileText("input angle results;")
          == completeCode(
              "argparse::ArgumentParser program(\"\");\nprogram.add_argument(\""
              "--results\")\n.required()\n.nargs(argparse::nargs_pattern::at_"
              "least_one)\n."
              "scan<'g', double>()\n.help(\"qasm input value of type: "
              "double|angle\");\ntry\n{\nprogram.parse_args(argc, "
              "argv);\n}\ncatch "
              "(const "
              "std::exception& err)\n{\nstd::cerr << err.what() << "
              "std::endl;\nstd::cerr << program;\nreturn -1;\n}\n"
              "QasmType<unsigned int, 32, true> "
              "results = program.get<QasmType<unsigned int, 32, "
              "true>>(\"results\");\n"));

    CHECK(compileText("input array[angle, 12] results;")
          == completeCode(
              "argparse::ArgumentParser program(\"\");\nprogram.add_argument(\""
              "--results\")\n.required()\n.nargs(argparse::nargs_pattern::at_"
              "least_one)\n."
              "scan<'g', double>()\n.help(\"qasm input value of type: "
              "array[double|angle]\");\ntry\n{\nprogram.parse_args(argc, "
              "argv);\n}\ncatch "
              "(const "
              "std::exception& err)\n{\nstd::cerr << err.what() << "
              "std::endl;\nstd::cerr << program;\nreturn -1;\n}\n"
              "std::vector<QasmType<unsigned int, 32, true>> "
              "results = program.get<std::vector<QasmType<unsigned int, 32, "
              "true>>>(\"results\");\n"));

    CHECK(compileText("input float results;")
          == completeCode(
              "argparse::ArgumentParser program(\"\");\nprogram.add_argument(\""
              "--results\")\n.required()\n.nargs(argparse::nargs_pattern::at_"
              "least_one)\n."
              "scan<'g', double>()\n.help(\"qasm input value of type: "
              "double|angle\");\ntry\n{\nprogram.parse_args(argc, "
              "argv);\n}\ncatch "
              "(const "
              "std::exception& err)\n{\nstd::cerr << err.what() << "
              "std::endl;\nstd::cerr << program;\nreturn -1;\n}\n"
              "double "
              "results = program.get<double>(\"results\");\n"));

    CHECK(compileText("input array[float, 14] results;")
          == completeCode(
              "argparse::ArgumentParser program(\"\");\nprogram.add_argument(\""
              "--results\")\n.required()\n.nargs(argparse::nargs_pattern::at_"
              "least_one)\n."
              "scan<'g', double>()\n.help(\"qasm input value of type: "
              "array[double|angle]\");\ntry\n{\nprogram.parse_args(argc, "
              "argv);\n}\ncatch "
              "(const "
              "std::exception& err)\n{\nstd::cerr << err.what() << "
              "std::endl;\nstd::cerr << program;\nreturn -1;\n}\n"
              "std::vector<double> "
              "results = program.get<std::vector<double>>(\"results\");\n"));

    CHECK_THROWS_WITH(
        compileText("input array[float, 14, 15] results;"),
        "<noname> \033[1;31merror: \033[0mmultidimensional inputs are not "
        "supported.\n1 | input array[float, 14, 15] results;\n");
}

TEST_CASE("testing old style quantum declaration statement.") {
    CHECK(compileText("qreg qubits[5];")
          == completeCode("auto qubits = QuregSlice(5);\n" + questCode("0 + 5")
                          + endQuestCode()));

    CHECK(compileText("creg reg[5];")
          == completeCode("QasmType<unsigned int, 5> reg(0);\n"));
}

TEST_CASE("testing quantum declaration statement.") {
    CHECK(compileText("qubit[5] qubits;")
          == completeCode("auto qubits = QuregSlice(5);\n" + questCode("0 + 5")
                          + endQuestCode()));

    CHECK(compileText("qubit[5] qubits;\nqubit[6] qubits2;")
          == completeCode("auto qubits = QuregSlice(5);\n"
                          "auto qubits2 = QuregSlice(6);\n"
                          + questCode("0 + 5 + 6") + endQuestCode()));

    CHECK_THROWS_WITH(compileText("qubit[0] qubits;"),
                      "<noname> \033[1;31merror: \033[0minvalid qubit "
                      "declaration. Number of qubits "
                      "should be positive.\n1 | qubit[0] qubits;\n");

    CHECK_THROWS_WITH(compileText("qubit[-10] qubits;"),
                      "<noname> \033[1;31merror: \033[0minvalid qubit "
                      "declaration. Number of qubits "
                      "should be positive.\n1 | qubit[-10] qubits;\n");
}

TEST_CASE("testing def statement.") {
    CHECK(compileText("def myfn(){}")
          == completeCode("", "void myfn()\n{\n}\n"));

    CHECK(
        compileText("def myfn() -> angle{}")
        == completeCode("", "QasmType<unsigned int, 32, true> myfn()\n{\n}\n"));

    CHECK(compileText("def myfn() -> float{}")
          == completeCode("", "double myfn()\n{\n}\n"));

    CHECK(compileText("def myfn() -> int{}")
          == completeCode("", "QasmType<int, 32> myfn()\n{\n}\n"));

    CHECK_THROWS_WITH(compileText("def myfn() -> qubit{}"),
                      "Encountered 1 errors while parsing.");

    CHECK_THROWS_WITH(compileText("def myfn() -> array[int, 5]{}"),
                      "Encountered 3 errors while parsing.");

    CHECK(compileText("def myfn() -> bool{}")
          == completeCode("", "QasmType<unsigned int, 1> myfn()\n{\n}\n"));

    CHECK(compileText("def myfn() -> complex{}")
          == completeCode("", "std::complex<double> myfn()\n{\n}\n"));

    CHECK(compileText("def myfn() -> uint{}")
          == completeCode("", "QasmType<unsigned int, 32> myfn()\n{\n}\n"));
    CHECK(compileText("def myfn() -> bit{}")
          == completeCode("", "QasmType<unsigned int, 1> myfn()\n{\n}\n"));

    CHECK(compileText("def myfn() -> duration{}")
          == completeCode("", "double myfn()\n{\n}\n"));
    CHECK(compileText("def myfn() -> stretch{}")
          == completeCode("", "double myfn()\n{\n}\n"));
}

TEST_CASE("testing extern statement") {
    CHECK_THROWS_WITH(
        compileText("extern foo();"),
        "<noname> \033[1;31merror: \033[0mextern statement is currently not "
        "supported.\n1 | extern foo();\n");
}

TEST_CASE("testing assignment statement") {
    CHECK(compileText("foo = measure q[0];")
          == completeCode(questCode("1") + "foo =measure(q[0]);\n"
                          + endQuestCode()));
    CHECK(compileText("foo[x, y, z] = measure q[0];")
          == completeCode(questCode("1")
                          + "foo[x][y][z] =one_qubit_measure(q[0]);\n"
                          + endQuestCode()));
    CHECK(compileText("foo[x, y, z] = bar;")
          == completeCode("foo[x][y][z] =bar;\n"));
    CHECK(compileText("foo = bar;") == completeCode("foo =bar;\n"));
    CHECK(compileText("foo += bar;") == completeCode("foo +=bar;\n"));
    CHECK(compileText("foo -= bar;") == completeCode("foo -=bar;\n"));
    CHECK(compileText("foo *= bar;") == completeCode("foo *=bar;\n"));
    CHECK(compileText("foo /= bar;") == completeCode("foo /=bar;\n"));
    CHECK(compileText("foo &= bar;") == completeCode("foo &=bar;\n"));
    CHECK(compileText("foo |= bar;") == completeCode("foo |=bar;\n"));
    CHECK(compileText("foo ^= bar;") == completeCode("foo ^=bar;\n"));
}

TEST_CASE("testing cal statement") {
    CHECK_THROWS_WITH(
        compileText("cal {}"),
        "<noname> \033[1;31merror: \033[0mcal statement is currently not "
        "supported.\n1 | cal {}\n");
}

TEST_CASE("testing defcal statement") {
    CHECK_THROWS_WITH(
        compileText("defcal foo() q -> bit {}"),
        "<noname> \033[1;31merror: \033[0mdefcal statement is currently not "
        "supported.\n1 | defcal foo() q -> bit {}\n");

    CHECK_THROWS_WITH(
        compileText("defcal foo q {}"),
        "<noname> \033[1;31merror: \033[0mdefcal statement is currently not "
        "supported.\n1 | defcal foo q {}\n");

    CHECK_THROWS_WITH(
        compileText("defcal foo(angle theta) q {}"),
        "<noname> \033[1;31merror: \033[0mdefcal statement is currently not "
        "supported.\n1 | defcal foo(angle theta) q {}\n");
}

TEST_CASE("testing parsing complex type variables.") {
    CHECK(
        compileText("complex[float[64]] comp;\n"
                    "comp = 2.5 + 3.5im;\n"
                    "complex[float] comp2 = 2.0 * sin(pi/2) + (3.1 * 5.5im);\n"
                    "float r = real(comp2);\n")
        == completeCode("comp =2.5+3.5_im;\n"
                        "std::complex<double> comp2 = 2.0 * "
                        "sin(3.14159265358979323846/2) + (3.1 * 5.5_im);\n"
                        "double r = real(comp2);\n",
                        "std::complex<double> comp;\n"));
}

TEST_CASE("testing parsing numeric literals.") {
    CHECK(compileText("int i = 1;")
          == completeCode("", "QasmType<int, 32> i(1);\n"));

    CHECK(compileText("int i = 0xff;")
          == completeCode("", "QasmType<int, 32> i(0xff);\n"));

    CHECK(compileText("int i = 0xffff_ffff;")
          == completeCode("", "QasmType<int, 32> i(0xffffffff);\n"));

    CHECK(compileText("int i = 0XBEEF;")
          == completeCode("", "QasmType<int, 32> i(0XBEEF);\n"));

    CHECK(compileText("int i = 0o73;")
          == completeCode("", "QasmType<int, 32> i(0o73);\n"));

    CHECK(compileText("int i = 0b1101;")
          == completeCode("", "QasmType<int, 32> i(0b1101);\n"));

    CHECK(compileText("int i = 0B0110_1001;")
          == completeCode("", "QasmType<int, 32> i(0B01101001);\n"));

    CHECK(compileText("int i = 1_000_000;")
          == completeCode("", "QasmType<int, 32> i(1000000);\n"));

    CHECK(compileText("duration sec = 1000ms;")
          == completeCode("", "double sec = 1000 * 1e-3;\n"));

    CHECK(compileText("duration hundrCycles = 100dt;")
          == completeCode("", "double hundrCycles = 100;\n"));
}

TEST_CASE("testing generation of arrays.") {
    CHECK(compileText(
              "array[int[32], 5, 2] foo = {{0, 1, 2, 3, 4}, {5, 6, 7, 8, 9}};")
          == completeCode("",
                          "std::vector<std::vector<QasmType<int, 32>>> foo = "
                          "{{0,1,2,3,4},{5,6,7,8,9}};\n"));

    SUBCASE("comma separated indices") {
        CHECK(compileText("foo = arr[0, 1];")
              == completeCode("foo =arr[0][1];\n"));

        CHECK(compileText("foo[3, 2] = arr[0, 1];")
              == completeCode("foo[3][2] =arr[0][1];\n"));

        CHECK(compileText("foo[3][2] = arr[0, 1];")
              == completeCode("foo[3][2] =arr[0][1];\n"));
    }

    SUBCASE("array slicing") {
        CHECK(compileText("foo = arr[0: 1];")
              == completeCode("foo =getSlice(arr, 0, 1, 1);\n"));

        CHECK(compileText("foo = arr[0: 2: 5];")
              == completeCode("foo =getSlice(arr, 0, 5, 2);\n"));

        CHECK(compileText("foo = arr[8: -2: 2];")
              == completeCode("foo =getSlice(arr, 8, 2, -2);\n"));

        CHECK(compileText("foo = arr[-6: -2: -1];")
              == completeCode("foo =getSlice(arr, -6, -1, -2);\n"));

        CHECK(compileText("foo = arr[-6: -1];")
              == completeCode("foo =getSlice(arr, -6, -1, 1);\n"));

        CHECK(compileText("foo = arr[-2: -6];")
              == completeCode("foo =getSlice(arr, -2, -6, 1);\n"));

        CHECK(compileText("foo = arr[-2: -6];")
              == completeCode("foo =getSlice(arr, -2, -6, 1);\n"));

        CHECK(compileText("foo = arr[3][0:4];")
              == completeCode("foo =getSlice(arr[3], 0, 4, 1);\n"));

        CHECK(compileText("foo = arr[0:4][3];")
              == completeCode("foo =getSlice(arr, 0, 4, 1)[3];\n"));
    }

    SUBCASE("assigning to slice") {
        CHECK(compileText("foo[0:2] = bar;")
              == completeCode("setSlice(foo, bar, 0, 2, 1);\n"));

        CHECK(compileText("foo[5:-1:3] = bar;")
              == completeCode("setSlice(foo, bar, 5, 3, -1);\n"));

        CHECK(compileText("foo[0:2] = bar[3:5];")
              == completeCode(
                  "setSlice(foo, getSlice(bar, 3, 5, 1), 0, 2, 1);\n"));

        CHECK_THROWS_WITH(
            compileText("foo[0:2, 1] = bar;"),
            "<noname> \033[1;31merror: \033[0mcurrently only 1D slices "
            "are allowed. From "
            "foo[0:2,1]=bar;\n1 | foo[0:2, 1] = bar;\n");

        CHECK_THROWS_WITH(
            compileText("foo[0:2][1] = bar;"),
            "<noname> \033[1;31merror: \033[0mcurrently only 1D slices "
            "are allowed. From "
            "foo[0:2][1]=bar;\n1 | foo[0:2][1] = bar;\n");
    }
}

TEST_CASE("testing bitwise xor expression") {
    CHECK(compileText("res = foo ^ bar;") == completeCode("res =foo^bar;\n"));
}

TEST_CASE("testing additive expression") {
    CHECK(compileText("res = foo + bar;") == completeCode("res =foo+bar;\n"));
    CHECK(compileText("res = foo - bar;") == completeCode("res =foo-bar;\n"));
    CHECK(compileText("res = -foo - bar;") == completeCode("res =-foo-bar;\n"));
    CHECK(compileText("res = -foo - -bar;")
          == completeCode("res =-foo--bar;\n"));
}

TEST_CASE("testing duration of expression") {
    CHECK_THROWS_WITH(
        compileText("durationof({});"),
        "<noname> \033[1;31merror: \033[0mdurationof expression is not "
        "supported.\n1 | durationof({})\n");
    CHECK_THROWS_WITH(
        compileText("durationof({x q[0];});"),
        "<noname> \033[1;31merror: \033[0mdurationof expression is not "
        "supported.\n1 | durationof({x q[0];})\n");
}

TEST_CASE("testing parenthesis expression") {
    CHECK(compileText("bar = (foo);") == completeCode("bar =(foo);\n"));
}

TEST_CASE("testing comparison expression") {
    CHECK(compileText("res = foo > bar;") == completeCode("res =foo>bar;\n"));
    CHECK(compileText("res = foo < bar;") == completeCode("res =foo<bar;\n"));
}

TEST_CASE("testing multiplicative expression") {
    CHECK(compileText("res = foo * bar;") == completeCode("res =foo*bar;\n"));
    CHECK(compileText("res = foo / bar;") == completeCode("res =foo/bar;\n"));
    CHECK(compileText("res = foo % bar;") == completeCode("res =foo%bar;\n"));
}

TEST_CASE("testing logical or expression") {
    CHECK(compileText("res = foo || bar;") == completeCode("res =foo||bar;\n"));
}

TEST_CASE("testing cast expression") {
    CHECK(compileText("bar = int(foo);")
          == completeCode("bar =static_cast<QasmType<int>>(foo);\n"));

    CHECK(compileText("bar = int[16](foo);")
          == completeCode("bar =static_cast<QasmType<int, 16>>(foo);\n"));

    CHECK_THROWS_WITH(compileText("bar = array[int, 5](foo);"),
                      "\033[1;31merror: \033[0mcasting array types is not "
                      "supported\n");
}

TEST_CASE("testing power expression") {
    CHECK(compileText("bar = a ** b;") == completeCode("bar =pow(a, b);\n"));
    CHECK(compileText("bar = (-5) ** b;")
          == completeCode("bar =pow((-5), b);\n"));
    CHECK(compileText("bar = 5 ** -42;")
          == completeCode("bar =pow(5, -42);\n"));
    CHECK(compileText("bar = 0 ** b;") == completeCode("bar =pow(0, b);\n"));
    CHECK(compileText("bar = 0 ** 0;") == completeCode("bar =pow(0, 0);\n"));
    CHECK(compileText("bar = a[0] ** b[4];")
          == completeCode("bar =pow(a[0], b[4]);\n"));

    CHECK(compileText("bar = c + a[0] ** b[4];")
          == completeCode("bar =c+pow(a[0], b[4]);\n"));

    CHECK(compileText("bar = (c - a[0]) ** b[4];")
          == completeCode("bar =pow((c-a[0]), b[4]);\n"));

    CHECK(
        compileText("for int i in [0 : 5 ** 2] {}")
        == completeCode("for (auto i = 0; i < pow(5, 2)  + 1; i += 1){\n}\n"));

    CHECK(compileText("while (k < 2 ** 5) {}")
          == completeCode("while (k<pow(2, 5))\n{\n}\n"));

    CHECK(compileText("if (k < 2 ** 5) {}")
          == completeCode("if (k<pow(2, 5))\n{\n}\n"));

    CHECK(compileText("def myfn(int a, int b, int c, int d) {}"
                      "myfn(2 ** 5, 45, q, 5 ** 4);")
          == completeCode("myfn(pow(2, 5), 45, q, pow(5, 4));\n",
                          "void myfn(QasmType<int, 32> a, QasmType<int, 32> b, "
                          "QasmType<int, 32> c, QasmType<int, 32> d)\n{\n}\n"));

    CHECK(compileText("bar = foo[2 ** 5];")
          == completeCode("bar =foo[pow(2, 5)];\n"));

    // this is too complex and breaks things
    //    CHECK(compileText("bar = foo[a[2 ** 5], 2 ** 3];") ==
    //          completeCode("bar =foo[pow(2, 5)];\n"));
}

TEST_CASE("testing bitwise or expression") {
    CHECK(compileText("bar = a[0] | b[4];")
          == completeCode("bar =a[0]|b[4];\n"));

    CHECK(compileText("bar = 123 | 42;") == completeCode("bar =123|42;\n"));
}

TEST_CASE("testing bit shift expression") {
    CHECK(compileText("bar = a[0] >> b[4];")
          == completeCode("bar =a[0]>>b[4];\n"));

    CHECK(compileText("bar = a[0]<<b[4];")
          == completeCode("bar =a[0]<<b[4];\n"));

    CHECK(compileText("bar = 32 >>3;") == completeCode("bar =32>>3;\n"));

    CHECK(compileText("bar = 14 << 4;") == completeCode("bar =14<<4;\n"));

    CHECK(compileText("bar = -32 >> 3;") == completeCode("bar =-32>>3;\n"));

    CHECK(compileText("bar = 14 << -4;") == completeCode("bar =14<<-4;\n"));

    CHECK(compileText("bar = 0 >> 3;") == completeCode("bar =0>>3;\n"));
}

TEST_CASE("testing bitwise and expression") {
    CHECK(compileText("bar = a[0] & b[4];")
          == completeCode("bar =a[0]&b[4];\n"));

    CHECK(compileText("bar = 32 & 3;") == completeCode("bar =32&3;\n"));

    CHECK(compileText("bar = -32 & 3;") == completeCode("bar =-32&3;\n"));

    CHECK(compileText("bar = 14 & -4;") == completeCode("bar =14&-4;\n"));

    CHECK(compileText("bar = 0 & 3;") == completeCode("bar =0&3;\n"));
}

TEST_CASE("testing equality expression") {
    CHECK(compileText("bar = a[0] == b[4];")
          == completeCode("bar =a[0]==b[4];\n"));

    CHECK(compileText("bar = a[0] != b[4];")
          == completeCode("bar =a[0]!=b[4];\n"));

    CHECK(compileText("bar = 32 == 3;") == completeCode("bar =32==3;\n"));

    CHECK(compileText("bar = 14 != 4;") == completeCode("bar =14!=4;\n"));

    CHECK(compileText("bar = -32 == 3;") == completeCode("bar =-32==3;\n"));

    CHECK(compileText("bar = 14 != -4;") == completeCode("bar =14!=-4;\n"));

    CHECK(compileText("bar = 0 == 3;") == completeCode("bar =0==3;\n"));
}

TEST_CASE("testing logical and expression") {
    CHECK(compileText("bar = a[0] && b[4];")
          == completeCode("bar =a[0]&&b[4];\n"));

    CHECK(compileText("bar = 32 && 3;") == completeCode("bar =32&&3;\n"));

    CHECK(compileText("bar = -32 && 3;") == completeCode("bar =-32&&3;\n"));

    CHECK(compileText("bar = 14 && -4;") == completeCode("bar =14&&-4;\n"));

    CHECK(compileText("bar = 0 && 3;") == completeCode("bar =0&&3;\n"));
}

TEST_CASE("testing argument definition list.") {
    CHECK(compileText("def my_func() -> angle[16]{}")
          == completeCode(
              "", "QasmType<unsigned int, 16, true> my_func()\n{\n}\n"));

    CHECK(
        compileText(
            "def my_func(angle arg1, readonly "
            "array[int[16], #dim = 1] arg2, readonly array[int[16], #dim = 3] "
            "arg3, "
            "int[16] "
            "arg4) -> "
            "angle[16]{}")
        == completeCode("",
                        "template <typename VecT0 , typename VecT1>\n"
                        "QasmType<unsigned int, 16, true> "
                        "my_func(QasmType<unsigned int, 32, true> arg1, "
                        "VecT0 && arg2, "
                        "VecT1 && arg3, QasmType<int, 16> "
                        "arg4)\n{\n}\n"));

    CHECK(compileText("def my_func(angle arg1, mutable "
                      "array[int[16], "
                      "#dim = 1] arg2) -> angle[16] {}")
          == completeCode("",
                          "template <typename VecT0 >\n"
                          "QasmType<unsigned int, 16, true> "
                          "my_func(QasmType<unsigned int, 32, true> arg1, "
                          "VecT0 && arg2)\n{\n}\n"));

    CHECK(compileText("def my_func(qubit[nqubits] arg1) -> angle[16] {}")
          == completeCode(
              "",
              "template <QasmQuregT T>\n"
              "QasmType<unsigned int, 16, true> my_func(T arg1)\n{\n}\n"));

    CHECK_THROWS_WITH(
        compileText("def my_func(angle arg1, readonly "
                    "array[int[16], #dim = foo] arg2, int[16] arg3) -> "
                    "angle[16]{}"),
        "<noname> \033[1;31merror: \033[0marray reference arguments in "
        "functions "
        "accept only literal number of dimensions. e.g. #dim = 1.\n1 | angle "
        "arg1, readonly array[int[16], #dim = foo] arg2, int[16] "
        "arg3\n");
}

TEST_CASE("testing call to the standard c++ compiler.") {
    auto str{ "def my_func(qubit[nqubits] arg1) -> angle[16] {}" };
    antlr4::ANTLRInputStream input{ str };
    qasm3Lexer lexer{ &input };
    antlr4::CommonTokenStream tokens(&lexer);
    qasm3Parser parser{ &tokens };
    auto error_strat = std::make_shared<CollectErrorStrategy>();
    parser.setErrorHandler(error_strat);
    qasm3Parser::ProgramContext* tree = nullptr;
    tree = parser.program();
    CollectErrorStrategy* errorStrat
        = dynamic_cast<CollectErrorStrategy*>(parser.getErrorHandler().get());
    errorStrat->reportErrors();

    Compiler compiler{};
    compiler.visitProgram(tree);

    const auto cppComp = "clang++";
    const auto outFile = "test_compiler_call";
    const auto libPath = std::format(" -L {}", DEPS_QUEST_LIB_DIR);
    const auto libs = std::format("");
    const std::string includePath = std::format(
        " -I {} -I {}", DEPS_QUEST_INCLUDE_DIR, DEPS_QASC_INCLUDE_DIR);
    const auto rpathPath = std::format(" -Wl,-rpath {}", DEPS_QUEST_LIB_DIR);
    const bool isFormatting = true;
    const bool isVerbose = true;
    CHECK_NOTHROW(compiler.compile(cppComp, outFile, libPath, libs, includePath,
                                   rpathPath, isFormatting, isVerbose));
}
