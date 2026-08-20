
// Generated from grammar/qasm3Parser.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"
#include "qasm3Parser.h"


namespace antlr4cpp {

/**
 * This interface defines an abstract listener for a parse tree produced by qasm3Parser.
 */
class  qasm3ParserListener : public antlr4::tree::ParseTreeListener {
public:

  virtual void enterProgram(qasm3Parser::ProgramContext *ctx) = 0;
  virtual void exitProgram(qasm3Parser::ProgramContext *ctx) = 0;

  virtual void enterVersion(qasm3Parser::VersionContext *ctx) = 0;
  virtual void exitVersion(qasm3Parser::VersionContext *ctx) = 0;

  virtual void enterStatement(qasm3Parser::StatementContext *ctx) = 0;
  virtual void exitStatement(qasm3Parser::StatementContext *ctx) = 0;

  virtual void enterAnnotation(qasm3Parser::AnnotationContext *ctx) = 0;
  virtual void exitAnnotation(qasm3Parser::AnnotationContext *ctx) = 0;

  virtual void enterScope(qasm3Parser::ScopeContext *ctx) = 0;
  virtual void exitScope(qasm3Parser::ScopeContext *ctx) = 0;

  virtual void enterPragma(qasm3Parser::PragmaContext *ctx) = 0;
  virtual void exitPragma(qasm3Parser::PragmaContext *ctx) = 0;

  virtual void enterStatementOrScope(qasm3Parser::StatementOrScopeContext *ctx) = 0;
  virtual void exitStatementOrScope(qasm3Parser::StatementOrScopeContext *ctx) = 0;

  virtual void enterCalibrationGrammarStatement(qasm3Parser::CalibrationGrammarStatementContext *ctx) = 0;
  virtual void exitCalibrationGrammarStatement(qasm3Parser::CalibrationGrammarStatementContext *ctx) = 0;

  virtual void enterIncludeStatement(qasm3Parser::IncludeStatementContext *ctx) = 0;
  virtual void exitIncludeStatement(qasm3Parser::IncludeStatementContext *ctx) = 0;

  virtual void enterBreakStatement(qasm3Parser::BreakStatementContext *ctx) = 0;
  virtual void exitBreakStatement(qasm3Parser::BreakStatementContext *ctx) = 0;

  virtual void enterContinueStatement(qasm3Parser::ContinueStatementContext *ctx) = 0;
  virtual void exitContinueStatement(qasm3Parser::ContinueStatementContext *ctx) = 0;

  virtual void enterEndStatement(qasm3Parser::EndStatementContext *ctx) = 0;
  virtual void exitEndStatement(qasm3Parser::EndStatementContext *ctx) = 0;

  virtual void enterForStatement(qasm3Parser::ForStatementContext *ctx) = 0;
  virtual void exitForStatement(qasm3Parser::ForStatementContext *ctx) = 0;

  virtual void enterIfStatement(qasm3Parser::IfStatementContext *ctx) = 0;
  virtual void exitIfStatement(qasm3Parser::IfStatementContext *ctx) = 0;

  virtual void enterReturnStatement(qasm3Parser::ReturnStatementContext *ctx) = 0;
  virtual void exitReturnStatement(qasm3Parser::ReturnStatementContext *ctx) = 0;

  virtual void enterWhileStatement(qasm3Parser::WhileStatementContext *ctx) = 0;
  virtual void exitWhileStatement(qasm3Parser::WhileStatementContext *ctx) = 0;

  virtual void enterSwitchStatement(qasm3Parser::SwitchStatementContext *ctx) = 0;
  virtual void exitSwitchStatement(qasm3Parser::SwitchStatementContext *ctx) = 0;

  virtual void enterSwitchCaseItem(qasm3Parser::SwitchCaseItemContext *ctx) = 0;
  virtual void exitSwitchCaseItem(qasm3Parser::SwitchCaseItemContext *ctx) = 0;

  virtual void enterBarrierStatement(qasm3Parser::BarrierStatementContext *ctx) = 0;
  virtual void exitBarrierStatement(qasm3Parser::BarrierStatementContext *ctx) = 0;

  virtual void enterBoxStatement(qasm3Parser::BoxStatementContext *ctx) = 0;
  virtual void exitBoxStatement(qasm3Parser::BoxStatementContext *ctx) = 0;

  virtual void enterDelayStatement(qasm3Parser::DelayStatementContext *ctx) = 0;
  virtual void exitDelayStatement(qasm3Parser::DelayStatementContext *ctx) = 0;

  virtual void enterGateCallStatement(qasm3Parser::GateCallStatementContext *ctx) = 0;
  virtual void exitGateCallStatement(qasm3Parser::GateCallStatementContext *ctx) = 0;

  virtual void enterMeasureArrowAssignmentStatement(qasm3Parser::MeasureArrowAssignmentStatementContext *ctx) = 0;
  virtual void exitMeasureArrowAssignmentStatement(qasm3Parser::MeasureArrowAssignmentStatementContext *ctx) = 0;

  virtual void enterResetStatement(qasm3Parser::ResetStatementContext *ctx) = 0;
  virtual void exitResetStatement(qasm3Parser::ResetStatementContext *ctx) = 0;

  virtual void enterAliasDeclarationStatement(qasm3Parser::AliasDeclarationStatementContext *ctx) = 0;
  virtual void exitAliasDeclarationStatement(qasm3Parser::AliasDeclarationStatementContext *ctx) = 0;

  virtual void enterClassicalDeclarationStatement(qasm3Parser::ClassicalDeclarationStatementContext *ctx) = 0;
  virtual void exitClassicalDeclarationStatement(qasm3Parser::ClassicalDeclarationStatementContext *ctx) = 0;

  virtual void enterConstDeclarationStatement(qasm3Parser::ConstDeclarationStatementContext *ctx) = 0;
  virtual void exitConstDeclarationStatement(qasm3Parser::ConstDeclarationStatementContext *ctx) = 0;

  virtual void enterIoDeclarationStatement(qasm3Parser::IoDeclarationStatementContext *ctx) = 0;
  virtual void exitIoDeclarationStatement(qasm3Parser::IoDeclarationStatementContext *ctx) = 0;

  virtual void enterOldStyleDeclarationStatement(qasm3Parser::OldStyleDeclarationStatementContext *ctx) = 0;
  virtual void exitOldStyleDeclarationStatement(qasm3Parser::OldStyleDeclarationStatementContext *ctx) = 0;

  virtual void enterQuantumDeclarationStatement(qasm3Parser::QuantumDeclarationStatementContext *ctx) = 0;
  virtual void exitQuantumDeclarationStatement(qasm3Parser::QuantumDeclarationStatementContext *ctx) = 0;

  virtual void enterDefStatement(qasm3Parser::DefStatementContext *ctx) = 0;
  virtual void exitDefStatement(qasm3Parser::DefStatementContext *ctx) = 0;

  virtual void enterExternStatement(qasm3Parser::ExternStatementContext *ctx) = 0;
  virtual void exitExternStatement(qasm3Parser::ExternStatementContext *ctx) = 0;

  virtual void enterGateStatement(qasm3Parser::GateStatementContext *ctx) = 0;
  virtual void exitGateStatement(qasm3Parser::GateStatementContext *ctx) = 0;

  virtual void enterAssignmentStatement(qasm3Parser::AssignmentStatementContext *ctx) = 0;
  virtual void exitAssignmentStatement(qasm3Parser::AssignmentStatementContext *ctx) = 0;

  virtual void enterExpressionStatement(qasm3Parser::ExpressionStatementContext *ctx) = 0;
  virtual void exitExpressionStatement(qasm3Parser::ExpressionStatementContext *ctx) = 0;

  virtual void enterCalStatement(qasm3Parser::CalStatementContext *ctx) = 0;
  virtual void exitCalStatement(qasm3Parser::CalStatementContext *ctx) = 0;

  virtual void enterDefcalStatement(qasm3Parser::DefcalStatementContext *ctx) = 0;
  virtual void exitDefcalStatement(qasm3Parser::DefcalStatementContext *ctx) = 0;

  virtual void enterBitwiseXorExpression(qasm3Parser::BitwiseXorExpressionContext *ctx) = 0;
  virtual void exitBitwiseXorExpression(qasm3Parser::BitwiseXorExpressionContext *ctx) = 0;

  virtual void enterAdditiveExpression(qasm3Parser::AdditiveExpressionContext *ctx) = 0;
  virtual void exitAdditiveExpression(qasm3Parser::AdditiveExpressionContext *ctx) = 0;

  virtual void enterDurationofExpression(qasm3Parser::DurationofExpressionContext *ctx) = 0;
  virtual void exitDurationofExpression(qasm3Parser::DurationofExpressionContext *ctx) = 0;

  virtual void enterParenthesisExpression(qasm3Parser::ParenthesisExpressionContext *ctx) = 0;
  virtual void exitParenthesisExpression(qasm3Parser::ParenthesisExpressionContext *ctx) = 0;

  virtual void enterComparisonExpression(qasm3Parser::ComparisonExpressionContext *ctx) = 0;
  virtual void exitComparisonExpression(qasm3Parser::ComparisonExpressionContext *ctx) = 0;

  virtual void enterMultiplicativeExpression(qasm3Parser::MultiplicativeExpressionContext *ctx) = 0;
  virtual void exitMultiplicativeExpression(qasm3Parser::MultiplicativeExpressionContext *ctx) = 0;

  virtual void enterLogicalOrExpression(qasm3Parser::LogicalOrExpressionContext *ctx) = 0;
  virtual void exitLogicalOrExpression(qasm3Parser::LogicalOrExpressionContext *ctx) = 0;

  virtual void enterCastExpression(qasm3Parser::CastExpressionContext *ctx) = 0;
  virtual void exitCastExpression(qasm3Parser::CastExpressionContext *ctx) = 0;

  virtual void enterPowerExpression(qasm3Parser::PowerExpressionContext *ctx) = 0;
  virtual void exitPowerExpression(qasm3Parser::PowerExpressionContext *ctx) = 0;

  virtual void enterBitwiseOrExpression(qasm3Parser::BitwiseOrExpressionContext *ctx) = 0;
  virtual void exitBitwiseOrExpression(qasm3Parser::BitwiseOrExpressionContext *ctx) = 0;

  virtual void enterCallExpression(qasm3Parser::CallExpressionContext *ctx) = 0;
  virtual void exitCallExpression(qasm3Parser::CallExpressionContext *ctx) = 0;

  virtual void enterBitshiftExpression(qasm3Parser::BitshiftExpressionContext *ctx) = 0;
  virtual void exitBitshiftExpression(qasm3Parser::BitshiftExpressionContext *ctx) = 0;

  virtual void enterBitwiseAndExpression(qasm3Parser::BitwiseAndExpressionContext *ctx) = 0;
  virtual void exitBitwiseAndExpression(qasm3Parser::BitwiseAndExpressionContext *ctx) = 0;

  virtual void enterEqualityExpression(qasm3Parser::EqualityExpressionContext *ctx) = 0;
  virtual void exitEqualityExpression(qasm3Parser::EqualityExpressionContext *ctx) = 0;

  virtual void enterLogicalAndExpression(qasm3Parser::LogicalAndExpressionContext *ctx) = 0;
  virtual void exitLogicalAndExpression(qasm3Parser::LogicalAndExpressionContext *ctx) = 0;

  virtual void enterIndexExpression(qasm3Parser::IndexExpressionContext *ctx) = 0;
  virtual void exitIndexExpression(qasm3Parser::IndexExpressionContext *ctx) = 0;

  virtual void enterUnaryExpression(qasm3Parser::UnaryExpressionContext *ctx) = 0;
  virtual void exitUnaryExpression(qasm3Parser::UnaryExpressionContext *ctx) = 0;

  virtual void enterLiteralExpression(qasm3Parser::LiteralExpressionContext *ctx) = 0;
  virtual void exitLiteralExpression(qasm3Parser::LiteralExpressionContext *ctx) = 0;

  virtual void enterAliasExpression(qasm3Parser::AliasExpressionContext *ctx) = 0;
  virtual void exitAliasExpression(qasm3Parser::AliasExpressionContext *ctx) = 0;

  virtual void enterDeclarationExpression(qasm3Parser::DeclarationExpressionContext *ctx) = 0;
  virtual void exitDeclarationExpression(qasm3Parser::DeclarationExpressionContext *ctx) = 0;

  virtual void enterMeasureExpression(qasm3Parser::MeasureExpressionContext *ctx) = 0;
  virtual void exitMeasureExpression(qasm3Parser::MeasureExpressionContext *ctx) = 0;

  virtual void enterRangeExpression(qasm3Parser::RangeExpressionContext *ctx) = 0;
  virtual void exitRangeExpression(qasm3Parser::RangeExpressionContext *ctx) = 0;

  virtual void enterSetExpression(qasm3Parser::SetExpressionContext *ctx) = 0;
  virtual void exitSetExpression(qasm3Parser::SetExpressionContext *ctx) = 0;

  virtual void enterArrayLiteral(qasm3Parser::ArrayLiteralContext *ctx) = 0;
  virtual void exitArrayLiteral(qasm3Parser::ArrayLiteralContext *ctx) = 0;

  virtual void enterIndexOperator(qasm3Parser::IndexOperatorContext *ctx) = 0;
  virtual void exitIndexOperator(qasm3Parser::IndexOperatorContext *ctx) = 0;

  virtual void enterIndexedIdentifier(qasm3Parser::IndexedIdentifierContext *ctx) = 0;
  virtual void exitIndexedIdentifier(qasm3Parser::IndexedIdentifierContext *ctx) = 0;

  virtual void enterReturnSignature(qasm3Parser::ReturnSignatureContext *ctx) = 0;
  virtual void exitReturnSignature(qasm3Parser::ReturnSignatureContext *ctx) = 0;

  virtual void enterGateModifier(qasm3Parser::GateModifierContext *ctx) = 0;
  virtual void exitGateModifier(qasm3Parser::GateModifierContext *ctx) = 0;

  virtual void enterScalarType(qasm3Parser::ScalarTypeContext *ctx) = 0;
  virtual void exitScalarType(qasm3Parser::ScalarTypeContext *ctx) = 0;

  virtual void enterQubitType(qasm3Parser::QubitTypeContext *ctx) = 0;
  virtual void exitQubitType(qasm3Parser::QubitTypeContext *ctx) = 0;

  virtual void enterArrayType(qasm3Parser::ArrayTypeContext *ctx) = 0;
  virtual void exitArrayType(qasm3Parser::ArrayTypeContext *ctx) = 0;

  virtual void enterArrayReferenceType(qasm3Parser::ArrayReferenceTypeContext *ctx) = 0;
  virtual void exitArrayReferenceType(qasm3Parser::ArrayReferenceTypeContext *ctx) = 0;

  virtual void enterDesignator(qasm3Parser::DesignatorContext *ctx) = 0;
  virtual void exitDesignator(qasm3Parser::DesignatorContext *ctx) = 0;

  virtual void enterDefcalTarget(qasm3Parser::DefcalTargetContext *ctx) = 0;
  virtual void exitDefcalTarget(qasm3Parser::DefcalTargetContext *ctx) = 0;

  virtual void enterDefcalArgumentDefinition(qasm3Parser::DefcalArgumentDefinitionContext *ctx) = 0;
  virtual void exitDefcalArgumentDefinition(qasm3Parser::DefcalArgumentDefinitionContext *ctx) = 0;

  virtual void enterDefcalOperand(qasm3Parser::DefcalOperandContext *ctx) = 0;
  virtual void exitDefcalOperand(qasm3Parser::DefcalOperandContext *ctx) = 0;

  virtual void enterGateOperand(qasm3Parser::GateOperandContext *ctx) = 0;
  virtual void exitGateOperand(qasm3Parser::GateOperandContext *ctx) = 0;

  virtual void enterExternArgument(qasm3Parser::ExternArgumentContext *ctx) = 0;
  virtual void exitExternArgument(qasm3Parser::ExternArgumentContext *ctx) = 0;

  virtual void enterArgumentDefinition(qasm3Parser::ArgumentDefinitionContext *ctx) = 0;
  virtual void exitArgumentDefinition(qasm3Parser::ArgumentDefinitionContext *ctx) = 0;

  virtual void enterArgumentDefinitionList(qasm3Parser::ArgumentDefinitionListContext *ctx) = 0;
  virtual void exitArgumentDefinitionList(qasm3Parser::ArgumentDefinitionListContext *ctx) = 0;

  virtual void enterDefcalArgumentDefinitionList(qasm3Parser::DefcalArgumentDefinitionListContext *ctx) = 0;
  virtual void exitDefcalArgumentDefinitionList(qasm3Parser::DefcalArgumentDefinitionListContext *ctx) = 0;

  virtual void enterDefcalOperandList(qasm3Parser::DefcalOperandListContext *ctx) = 0;
  virtual void exitDefcalOperandList(qasm3Parser::DefcalOperandListContext *ctx) = 0;

  virtual void enterExpressionList(qasm3Parser::ExpressionListContext *ctx) = 0;
  virtual void exitExpressionList(qasm3Parser::ExpressionListContext *ctx) = 0;

  virtual void enterIdentifierList(qasm3Parser::IdentifierListContext *ctx) = 0;
  virtual void exitIdentifierList(qasm3Parser::IdentifierListContext *ctx) = 0;

  virtual void enterGateOperandList(qasm3Parser::GateOperandListContext *ctx) = 0;
  virtual void exitGateOperandList(qasm3Parser::GateOperandListContext *ctx) = 0;

  virtual void enterExternArgumentList(qasm3Parser::ExternArgumentListContext *ctx) = 0;
  virtual void exitExternArgumentList(qasm3Parser::ExternArgumentListContext *ctx) = 0;


};

}  // namespace antlr4cpp
