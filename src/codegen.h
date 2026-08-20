/**
 * @copyright Copyright (c) 2025-2026
 * UK Research and Innovation,
 * Science and Technology Facilities Council,
 * Hartree Centre
 **/

#ifndef QASC_CODEGEN_H
#define QASC_CODEGEN_H

#include <string>
#include <string_view>
#include <vector>

struct Output;
using Input = Output;

namespace Generator {
using Instr = std::string;
using StrView = std::string_view;

Instr emitDepsHeaders();

Instr emitIncludeCode(StrView headerDef, Instr&& code);

Instr emitBreakStmt();

Instr emitContinueStmt();

Instr emitEndStmt();

Instr emitLeftBracket();

Instr emitRightBracket();

Instr emitLeftParen();

Instr emitRightParen();

Instr emitForStmt(StrView indexId,
                  StrView start,
                  StrView end,
                  StrView increment);

Instr emitForStmt(StrView indexId, StrView vals);

Instr emitIfStmt(StrView expression);

Instr emitElseStmt();

Instr emitReturnStmt(StrView expression);

Instr emitWhileStmt(StrView expression);

Instr emitSwitchStmt(StrView expression);

Instr emitCaseStmt(StrView expression);

Instr emitDefaultStmt();

Instr emitGateCallStmt(StrView gateName, StrView qregName, StrView target);

Instr emitGateCallStmt(StrView gateName,
                       StrView qregName,
                       StrView target,
                       StrView ctrlOrAngle);

Instr emitGateCallStmt(StrView gateName,
                       StrView qregName,
                       StrView target,
                       StrView ctrl,
                       StrView angle);

Instr emitGateCallStmt(StrView gateName,
                       StrView qregName,
                       StrView target,
                       StrView angle1,
                       StrView angle2,
                       StrView angle3,
                       bool isInverse = false);

Instr emitGateCallStmt(StrView gateName,
                       StrView qregName,
                       StrView target,
                       StrView ctrl,
                       StrView angle1,
                       StrView angle2,
                       StrView angle3,
                       StrView angle4);

Instr emitGPhaseStmt(StrView numQubits, StrView qregName, StrView angle);

Instr emitResetStmt(StrView qregName);

Instr emitResetStmt(StrView qregName, StrView target);

Instr emitAliasDecl(StrView aliasName, StrView expression);

Instr emitClassicalDeclStmt(StrView type, StrView varName, StrView expression);

Instr emitClassicalDeclFront(StrView type, StrView varName);

Instr emitClassicalDeclBack();

Instr emitQuantumDeclStmt(StrView qregName, StrView expression);

Instr emitQuestInit();

Instr emitQuestFree();

Instr emitConstMod(bool startedMain);

Instr emitIoDeclStmt(StrView type, StrView varName, StrView expression);

Instr emitFuncDefStmt(StrView returnType, StrView fnName, StrView args);

Instr emitOutput(StrView outputFilename, const std::vector<Output>& outputs);

Instr emitInputs(const std::vector<Input>& inputs);

Instr emitAssignmentStmt(StrView varName, StrView assignOp, StrView expression);

Instr emitAssignmentFront(StrView varName, StrView assignOp);

Instr emitAssignmentBack();

Instr emitCastFront(StrView type);

Instr emitCastBack();

Instr emitFuncCallStmt(StrView funcName, StrView argList);

Instr emitExprStmt(StrView expression);

Instr emitMeasurement(StrView qregName, StrView target);

// operators
Instr emitPlusOp();

Instr emitMinusOp();

Instr emitMultOp();

Instr emitDivOp();

Instr emitModOp();

Instr emitXorOp();

Instr emitCompOp(StrView op);

Instr emitDoublePipeOp();

Instr emitPipeOp();

Instr emitBitshiftOp(StrView op);

Instr emitAmpersandOp();

Instr emitEqualityOp(StrView op);

Instr emitDoubleAmpersandOp();

Instr emitMinusUnary();

Instr emitExclamationUnary();

Instr emitTildeUnary();

Instr emitPowOp(StrView lhs, StrView rhs);

Instr emitLiteral(StrView literal);

Instr emitScalarType(std::string scalarType);

Instr emitNdArray(int numDim, StrView type);

}    // namespace Generator

#endif    // !QASC_CODEGEN_H
