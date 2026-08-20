/**
 * @copyright Copyright (c) 2025-2026
 * UK Research and Innovation,
 * Science and Technology Facilities Council,
 * Hartree Centre
 **/

#include "qubitCountPass.h"

#include "utils.h"

#include <string>

using namespace antlr4cpp;

std::string QubitCountPass::totalNumQubits() const {
    return totalNumQubits_;
}

std::string QubitCountPass::lastQuregName() const {
    return lastQuregName_;
}

std::any QubitCountPass::visitProgram(qasm3Parser::ProgramContext* ctx) {
    return visitChildren(ctx);
}

std::any QubitCountPass::visitIncludeStatement(
    qasm3Parser::IncludeStatementContext* ctx) {
    return visitChildren(ctx);
}

std::any QubitCountPass::visitOldStyleDeclarationStatement(
    qasm3Parser::OldStyleDeclarationStatementContext* ctx) {
    if (ctx->QREG()) {
        auto expression = ctx->designator()->expression()->getText();
        expression = Utils::replacePowerOp(expression);
        expression = Utils::replaceAllCastExpr(expression);
        totalNumQubits_ += " + " + expression;
        lastQuregName_ = ctx->Identifier()->getText();
    }

    return visitChildren(ctx);
}

std::any QubitCountPass::visitQuantumDeclarationStatement(
    qasm3Parser::QuantumDeclarationStatementContext* ctx) {
    std::string expression = "1";

    if (ctx->qubitType()->designator() &&
        ctx->qubitType()->designator()->expression()) {
        expression = ctx->qubitType()->designator()->expression()->getText();
        expression = Utils::replacePowerOp(expression);
        expression = Utils::replaceAllCastExpr(expression);
    }

    totalNumQubits_ += " + " + expression;
    lastQuregName_ = ctx->Identifier()->getText();

    return visitChildren(ctx);
}
