/**
 * @copyright Copyright (c) 2025-2026
 * UK Research and Innovation,
 * Science and Technology Facilities Council,
 * Hartree Centre
 **/

#ifndef QASC_QUBIT_COUNT_PASS_H
#define QASC_QUBIT_COUNT_PASS_H

#include "qasm3Parser.h"
#include "qasm3ParserBaseVisitor.h"

#include <string>

using namespace antlr4cpp;
class QubitCountPass : public antlr4cpp::qasm3ParserBaseVisitor {
   public:
    explicit QubitCountPass() {};

    std::string totalNumQubits() const;
    std::string lastQuregName() const;

    virtual std::any visitProgram(qasm3Parser::ProgramContext* ctx) override;

    virtual std::any visitIncludeStatement(
        qasm3Parser::IncludeStatementContext* ctx) override;

    virtual std::any visitOldStyleDeclarationStatement(
        qasm3Parser::OldStyleDeclarationStatementContext* ctx) override;

    virtual std::any visitQuantumDeclarationStatement(
        qasm3Parser::QuantumDeclarationStatementContext* ctx) override;

   private:
    std::string totalNumQubits_{"0"};
    std::string lastQuregName_{""};
};

#endif    // !QASC_QUBIT_COUNT_PASS_H
