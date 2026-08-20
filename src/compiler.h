/**
 * @copyright Copyright (c) 2025-2026
 * UK Research and Innovation,
 * Science and Technology Facilities Council,
 * Hartree Centre
 **/

#ifndef QASC_COMPILER_H
#define QASC_COMPILER_H

#include "codegen.h"
#include "qasm3Parser.h"
#include "qasm3ParserBaseVisitor.h"
#include "utils.h"

#include <cassert>
#include <cstddef>
#include <format>
#include <string>
#include <vector>

class CodeChunk {
   public:
    void add(std::string& instr);
    void add(std::string&& instr);
    void pop();
    void print() const;
    void clear();
    std::string last() const;
    ssize_t size() const;
    void insert(ssize_t index, std::string&& value);
    void replace(size_t index, const std::string& value);
    std::string toStr() const;
    std::string toStrTo(size_t lineNumber) const;
    std::string toStrFrom(size_t lineNumber) const;

    std::string& at(size_t index) {
        assert(index < code_.size() &&
               "indexing CodeChunk should always be valid");
        return code_[index];
    };

   private:
    std::vector<std::string> code_;
};

struct FuncSignature {
    std::string fnName{""};
    std::string returnType{""};
    std::vector<std::string> argTypes{};
    bool callsGates{false};
};

using namespace antlr4cpp;

class Compiler : public antlr4cpp::qasm3ParserBaseVisitor {
   public:
    using StrView = std::string_view;

    explicit Compiler() {};
    explicit Compiler(bool isCallingInclude)
        : callingFromInclude_(isCallingInclude) {};

    explicit Compiler(const std::string& inputFilename,
                      const std::string& outputFilename)
        : inputFilename_(inputFilename),
          outputFilename_(outputFilename),
          currentFilename_(inputFilename) {};

    virtual std::any visitProgram(qasm3Parser::ProgramContext* ctx) override;
    virtual std::any visitVersion(qasm3Parser::VersionContext* ctx) override;
    virtual std::any visitStatement(
        qasm3Parser::StatementContext* ctx) override;
    virtual std::any visitAnnotation(
        qasm3Parser::AnnotationContext* ctx) override;
    virtual std::any visitScope(qasm3Parser::ScopeContext* ctx) override;
    virtual std::any visitPragma(qasm3Parser::PragmaContext* ctx) override;
    virtual std::any visitStatementOrScope(
        qasm3Parser::StatementOrScopeContext* ctx) override;
    virtual std::any visitCalibrationGrammarStatement(
        qasm3Parser::CalibrationGrammarStatementContext* ctx) override;
    virtual std::any visitIncludeStatement(
        qasm3Parser::IncludeStatementContext* ctx) override;
    virtual std::any visitBreakStatement(
        qasm3Parser::BreakStatementContext* ctx) override;
    virtual std::any visitContinueStatement(
        qasm3Parser::ContinueStatementContext* ctx) override;
    virtual std::any visitEndStatement(
        qasm3Parser::EndStatementContext* ctx) override;
    virtual std::any visitForStatement(
        qasm3Parser::ForStatementContext* ctx) override;
    virtual std::any visitIfStatement(
        qasm3Parser::IfStatementContext* ctx) override;
    virtual std::any visitReturnStatement(
        qasm3Parser::ReturnStatementContext* ctx) override;
    virtual std::any visitWhileStatement(
        qasm3Parser::WhileStatementContext* ctx) override;
    virtual std::any visitSwitchStatement(
        qasm3Parser::SwitchStatementContext* ctx) override;
    virtual std::any visitSwitchCaseItem(
        qasm3Parser::SwitchCaseItemContext* ctx) override;
    virtual std::any visitBarrierStatement(
        qasm3Parser::BarrierStatementContext* ctx) override;
    virtual std::any visitBoxStatement(
        qasm3Parser::BoxStatementContext* ctx) override;
    virtual std::any visitDelayStatement(
        qasm3Parser::DelayStatementContext* ctx) override;
    virtual std::any visitGateCallStatement(
        qasm3Parser::GateCallStatementContext* ctx) override;
    virtual std::any visitMeasureArrowAssignmentStatement(
        qasm3Parser::MeasureArrowAssignmentStatementContext* ctx) override;
    virtual std::any visitResetStatement(
        qasm3Parser::ResetStatementContext* ctx) override;
    virtual std::any visitAliasDeclarationStatement(
        qasm3Parser::AliasDeclarationStatementContext* ctx) override;
    virtual std::any visitClassicalDeclarationStatement(
        qasm3Parser::ClassicalDeclarationStatementContext* ctx) override;
    virtual std::any visitConstDeclarationStatement(
        qasm3Parser::ConstDeclarationStatementContext* ctx) override;
    virtual std::any visitIoDeclarationStatement(
        qasm3Parser::IoDeclarationStatementContext* ctx) override;
    virtual std::any visitOldStyleDeclarationStatement(
        qasm3Parser::OldStyleDeclarationStatementContext* ctx) override;
    virtual std::any visitQuantumDeclarationStatement(
        qasm3Parser::QuantumDeclarationStatementContext* ctx) override;
    virtual std::any visitDefStatement(
        qasm3Parser::DefStatementContext* ctx) override;
    virtual std::any visitExternStatement(
        qasm3Parser::ExternStatementContext* ctx) override;
    virtual std::any visitGateStatement(
        qasm3Parser::GateStatementContext* ctx) override;
    virtual std::any visitAssignmentStatement(
        qasm3Parser::AssignmentStatementContext* ctx) override;
    virtual std::any visitExpressionStatement(
        qasm3Parser::ExpressionStatementContext* ctx) override;
    virtual std::any visitCalStatement(
        qasm3Parser::CalStatementContext* ctx) override;
    virtual std::any visitDefcalStatement(
        qasm3Parser::DefcalStatementContext* ctx) override;
    virtual std::any visitBitwiseXorExpression(
        qasm3Parser::BitwiseXorExpressionContext* ctx) override;
    virtual std::any visitAdditiveExpression(
        qasm3Parser::AdditiveExpressionContext* ctx) override;
    virtual std::any visitDurationofExpression(
        qasm3Parser::DurationofExpressionContext* ctx) override;
    virtual std::any visitParenthesisExpression(
        qasm3Parser::ParenthesisExpressionContext* ctx) override;
    virtual std::any visitComparisonExpression(
        qasm3Parser::ComparisonExpressionContext* ctx) override;
    virtual std::any visitMultiplicativeExpression(
        qasm3Parser::MultiplicativeExpressionContext* ctx) override;
    virtual std::any visitLogicalOrExpression(
        qasm3Parser::LogicalOrExpressionContext* ctx) override;
    virtual std::any visitCastExpression(
        qasm3Parser::CastExpressionContext* ctx) override;
    virtual std::any visitPowerExpression(
        qasm3Parser::PowerExpressionContext* ctx) override;
    virtual std::any visitBitwiseOrExpression(
        qasm3Parser::BitwiseOrExpressionContext* ctx) override;
    virtual std::any visitCallExpression(
        qasm3Parser::CallExpressionContext* ctx) override;
    virtual std::any visitBitshiftExpression(
        qasm3Parser::BitshiftExpressionContext* ctx) override;
    virtual std::any visitBitwiseAndExpression(
        qasm3Parser::BitwiseAndExpressionContext* ctx) override;
    virtual std::any visitEqualityExpression(
        qasm3Parser::EqualityExpressionContext* ctx) override;
    virtual std::any visitLogicalAndExpression(
        qasm3Parser::LogicalAndExpressionContext* ctx) override;
    virtual std::any visitIndexExpression(
        qasm3Parser::IndexExpressionContext* ctx) override;
    virtual std::any visitUnaryExpression(
        qasm3Parser::UnaryExpressionContext* ctx) override;
    virtual std::any visitLiteralExpression(
        qasm3Parser::LiteralExpressionContext* ctx) override;
    virtual std::any visitAliasExpression(
        qasm3Parser::AliasExpressionContext* ctx) override;
    virtual std::any visitDeclarationExpression(
        qasm3Parser::DeclarationExpressionContext* ctx) override;
    virtual std::any visitMeasureExpression(
        qasm3Parser::MeasureExpressionContext* ctx) override;
    virtual std::any visitRangeExpression(
        qasm3Parser::RangeExpressionContext* ctx) override;
    virtual std::any visitSetExpression(
        qasm3Parser::SetExpressionContext* ctx) override;
    virtual std::any visitArrayLiteral(
        qasm3Parser::ArrayLiteralContext* ctx) override;
    virtual std::any visitIndexOperator(
        qasm3Parser::IndexOperatorContext* ctx) override;
    virtual std::any visitIndexedIdentifier(
        qasm3Parser::IndexedIdentifierContext* ctx) override;
    virtual std::any visitReturnSignature(
        qasm3Parser::ReturnSignatureContext* ctx) override;
    virtual std::any visitGateModifier(
        qasm3Parser::GateModifierContext* ctx) override;
    virtual std::any visitScalarType(
        qasm3Parser::ScalarTypeContext* ctx) override;
    virtual std::any visitQubitType(
        qasm3Parser::QubitTypeContext* ctx) override;
    virtual std::any visitArrayType(
        qasm3Parser::ArrayTypeContext* ctx) override;
    virtual std::any visitArrayReferenceType(
        qasm3Parser::ArrayReferenceTypeContext* ctx) override;
    virtual std::any visitDesignator(
        qasm3Parser::DesignatorContext* ctx) override;
    virtual std::any visitDefcalTarget(
        qasm3Parser::DefcalTargetContext* ctx) override;
    virtual std::any visitDefcalArgumentDefinition(
        qasm3Parser::DefcalArgumentDefinitionContext* ctx) override;
    virtual std::any visitDefcalOperand(
        qasm3Parser::DefcalOperandContext* ctx) override;
    virtual std::any visitGateOperand(
        qasm3Parser::GateOperandContext* ctx) override;
    virtual std::any visitExternArgument(
        qasm3Parser::ExternArgumentContext* ctx) override;
    virtual std::any visitArgumentDefinition(
        qasm3Parser::ArgumentDefinitionContext* ctx) override;
    virtual std::any visitArgumentDefinitionList(
        qasm3Parser::ArgumentDefinitionListContext* ctx) override;
    virtual std::any visitDefcalArgumentDefinitionList(
        qasm3Parser::DefcalArgumentDefinitionListContext* ctx) override;
    virtual std::any visitDefcalOperandList(
        qasm3Parser::DefcalOperandListContext* ctx) override;
    virtual std::any visitExpressionList(
        qasm3Parser::ExpressionListContext* ctx) override;
    virtual std::any visitIdentifierList(
        qasm3Parser::IdentifierListContext* ctx) override;
    virtual std::any visitGateOperandList(
        qasm3Parser::GateOperandListContext* ctx) override;
    virtual std::any visitExternArgumentList(
        qasm3Parser::ExternArgumentListContext* ctx) override;

    void firstPass(qasm3Parser::ProgramContext* ctx);

    std::string extractScalarType(antlr4::ParserRuleContext* ctx,
                                  std::string scalarType);

    std::string getCode() const;
    std::string getCodeTo(size_t lineNumber) const;
    std::string getCodeFrom(size_t lineNumber) const;

    std::string warningMessage(antlr4::ParserRuleContext* ctx,
                               const std::string& msg);
    std::string errorMessage(antlr4::ParserRuleContext* ctx,
                             const std::string& msg);

    void compile(const std::string& cppComp,
                 const std::string& outFile,
                 const std::string& libPath,
                 const std::string& libs,
                 const std::string& includePath,
                 const std::string& rpathPath,
                 const bool isFormatting,
                 const bool isVerbose);

    template <typename T>
    bool generateScalarDeclaration(T* ctx) {
        std::string type{extractScalarType(ctx, ctx->scalarType()->getText())};
        if (type == "float") {
            type = "double";
        }
        const auto id{ctx->Identifier()->getText()};

        if (type != "double" && type != "std::complex<double>") {
            std::string value = "0";
            if (ctx->declarationExpression()) {
                if (ctx->declarationExpression()->expression()) {
                    value =
                        ctx->declarationExpression()->expression()->getText();
                } else if (ctx->declarationExpression()->measureExpression()) {
                    value = "measure(" +
                            ctx->declarationExpression()
                                ->measureExpression()
                                ->gateOperand()
                                ->getText() +
                            ")";
                }
                value = Utils::processExpression(value);
            }

            std::string bitArraySize = "32";

            std::string isAngle = "";

            if (type.find(", true>") != type.npos) {
                isAngle = ", true";
            }

            if (ctx->scalarType()->BIT() || ctx->scalarType()->BOOL()) {
                bitArraySize = "1";
            }

            if (ctx->scalarType()->designator() &&
                ctx->scalarType()->designator()->expression()) {
                bitArraySize =
                    ctx->scalarType()->designator()->expression()->getText();
            }

            auto commaPos = type.find(',');
            type = type.substr(0, commaPos + 1);

            type += std::format(" {}{}>", bitArraySize, isAngle);
            value = Utils::replaceSlice(value);
            const auto defaultInitializer = std::format("({})", value);

            code_.add(
                Generator::emitClassicalDeclStmt(type, id, defaultInitializer));
            if (scopeLvl_ == 0) {
                potentialOutputs_.emplace_back(Output{type, id, 0});
            }
            return true;
        }

        code_.add(Generator::emitClassicalDeclFront(type, id));
        code_.add(" = ");
        if (ctx->declarationExpression()) {
            ctx->declarationExpression()->expression()->accept(this);
        }
        if (code_.last() == " = ") {
            code_.pop();
        }
        code_.add(Generator::emitClassicalDeclBack());
        if (scopeLvl_ == 0) {
            potentialOutputs_.emplace_back(Output{type, id, 0});
        }
        return true;
    }

    template <typename T>
    bool generateArrayDeclaration(T* ctx) {
        const int numDim =
            ctx->arrayType()->expressionList()->expression().size();
        std::string type =
            extractScalarType(ctx, ctx->arrayType()->scalarType()->getText());
        if (type == "float") {
            type = "double";
        }

        if (type != "double" && type != "std::complex<double>") {
            std::string bitArraySize = "32";

            std::string isAngle = "";

            if (type.find(", true>") != type.npos) {
                isAngle = ", true";
            }

            if (ctx->arrayType()->scalarType()->BIT() ||
                ctx->arrayType()->scalarType()->BOOL()) {
                bitArraySize = "1";
            }

            if (ctx->arrayType()->scalarType()->designator() &&
                ctx->arrayType()->scalarType()->designator()->expression()) {
                bitArraySize = ctx->arrayType()
                                   ->scalarType()
                                   ->designator()
                                   ->expression()
                                   ->getText();
            }
            auto commaPos = type.find(',');
            type = type.substr(0, commaPos + 1);

            type += std::format(" {}{}>", bitArraySize, isAngle);
        }

        const auto id{ctx->Identifier()->getText()};

        std::string expression{""};
        if (ctx->declarationExpression()) {
            expression = ctx->declarationExpression()->getText();
            expression = Utils::processExpression(expression);
            code_.add(Generator::emitClassicalDeclStmt(
                Generator::emitNdArray(numDim, type), id, expression));
            if (scopeLvl_ == 0) {
                potentialOutputs_.emplace_back(Output{type, id, numDim});
            }
            return true;
        }

        std::string defaultInitializer{""};
        for (int i = 0; i < numDim; ++i) {
            auto expr{
                ctx->arrayType()->expressionList()->expression(i)->getText()};
            std::string sndArg{", 0)"};    // default 0 initialization
            if (numDim - i - 1 != 0) {
                sndArg = ", " + Generator::emitNdArray(numDim - i - 1, type);
            }
            defaultInitializer += "(" + expr + sndArg;
        }

        for (int i = 1; i < numDim; ++i) {
            defaultInitializer += ")";
        }

        code_.add(Generator::emitClassicalDeclStmt(
            Generator::emitNdArray(numDim, type), id, defaultInitializer));

        if (scopeLvl_ == 0) {
            potentialOutputs_.emplace_back(Output{type, id, numDim});
        }
        return true;
    }

    std::string totalNumQubits() const {
        return totalNumQubits_;
    }

    void getDefinedFuncsFromInclude(
        const std::vector<FuncSignature>& otherDefinedFuncs);

    int includeMainLineNumber() const {
        return includeMainLineNumber_;
    }

   private:
    struct QregInfo {
        std::string name{""};
        std::string numQubitsExpr{""};
    };

    std::string totalNumQubits_{"0"};
    std::string lastQuregName_{""};

    int scopeLvl_{0};
    bool startedMain_{false};
    int firstLineInMainIdx_{0};
    CodeChunk code_{};
    std::vector<Output> outputs_{};
    std::vector<Output> potentialOutputs_{};
    std::vector<Input> inputs_{};
    std::string inputFilename_{"<noname>"};
    std::string outputFilename_{"<noname>"};
    std::string currentFilename_{"<noname>"};
    std::string tempFilename_{"<noname>"};
    bool isQbackendInit_{false};

    bool callingFromInclude_{false};
    int includeMainLineNumber_{-1};
    std::vector<std::string> includeCodeForMainFn_{};

    std::string currentAssignmentName_{""};
    bool callingFromAliasDeclaration_{false};
    bool callingFromGateDeclaration_{false};

    bool callingFromMainFn_{false};
    bool callingFromDefStmt_{false};
    bool callingFromAssignment_{false};
    bool callingFromDeclaration_{false};
    bool processingSlice_{false};
    bool processingRHSAsSlice_{false};
    bool applyingInverse_{false};

    std::vector<FuncSignature> definedFuncs_{
        {"cos", "double", {"double"}},
        {"sin", "double", {"double"}},
        {"tan", "double", {"double"}},
        {"arcsin", "double", {"double"}},
        {"arccos", "double", {"double"}},
        {"arctan", "double", {"double"}},
        {"ceiling", "double", {"double"}},
        {"floor", "double", {"double"}},
        {"exp", "double", {"double"}},
        {"log", "double", {"double"}},
        {"mod", "double", {"double", "double"}},
        {"sqrt", "double", {"double"}},
        {"pow", "double", {"double", "double"}},
        {"sizeof", "size_t", {"", ""}},
        {"real", "double", {"std::complex<double>"}},
        {"imag", "double", {"std::complex<double>"}},
        {"rotl", "", {"", "int"}},
        {"rotr", "", {"", "int"}},
        {"popcount", "ssize_t", {""}}

    };

    std::vector<QregInfo> qregInfos_{};

    void initQuantumBackend();

    void tryRegisterMainFn();

    void tryFinalizingMainFn();

    void handleGateModifier(
        const std::vector<qasm3Parser::GateModifierContext*>& modifiers,
        std::string& gateName,
        int& gatePower,
        std::string& gatePowerStr);

    std::vector<std::string> getCodeFromIncludes();
};

#endif    // !QASC_COMPILER_H
