/**
 * @copyright Copyright (c) 2025-2026
 * UK Research and Innovation,
 * Science and Technology Facilities Council,
 * Hartree Centre
 **/

#include "compiler.h"

#include "codegen.h"
#include "qasm3Lexer.h"
#include "qasm3Parser.h"
#include "qubitCountPass.h"
#include "utils.h"

#include <algorithm>
#include <any>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
const std::string qascInternalQuESTQureg{ "qascInternalQuestQureg" };

enum class OpType { QASC_UNARY_EXPRESSION, QASC_BINARY_EXPRESSION };

}    // namespace

std::string Compiler::warningMessage(antlr4::ParserRuleContext* ctx,
                                     const std::string& msg) {
    if (ctx == nullptr) {
        throw std::runtime_error(std::string("\033[1;31m")
                                 + "error: " + "\033[0m" + "null context.\n");
    }

    std::string error = currentFilename_ + " " + std::string("\033[1;35m")
                        + "warning: " + "\033[0m" + msg + "\n"
                        + std::to_string(ctx->getStart()->getLine()) + " | ";

    ssize_t startIdx = ctx->start->getStartIndex();
    ssize_t stopIdx = ctx->stop->getStopIndex();
    error += ctx->start->getInputStream()->getText(
                 antlr4::misc::Interval(startIdx, stopIdx))
             + "\n";

    return (error);
}

std::string Compiler::errorMessage(antlr4::ParserRuleContext* ctx,
                                   const std::string& msg) {
    std::string error;

    if (ctx == nullptr) {
        error = std::string("\033[1;31m") + "error: " + "\033[0m"
                + "null context.\n";

    } else {
        error = currentFilename_ + " " + std::string("\033[1;31m")
                + "error: " + "\033[0m" + msg + "\n"
                + std::to_string(ctx->getStart()->getLine()) + " | ";

        ssize_t startIdx = ctx->start->getStartIndex();
        ssize_t stopIdx = ctx->stop->getStopIndex();
        error += ctx->start->getInputStream()->getText(
                     antlr4::misc::Interval(startIdx, stopIdx))
                 + "\n";
    }

    return (error);
}

void CodeChunk::add(std::string& instr) {
    code_.emplace_back(instr);
}

void CodeChunk::add(std::string&& instr) {
    code_.emplace_back(instr);
}

void CodeChunk::pop() {
    code_.pop_back();
}

void CodeChunk::print() const {
    for (auto& instr : code_) {
        Utils::print(std::format("{}", instr));
    }
}

void CodeChunk::clear() {
    code_.clear();
}

std::string CodeChunk::last() const {
    return code_.back();
}

ssize_t CodeChunk::size() const {
    return code_.size();
}

void CodeChunk::insert(ssize_t index, std::string&& value) {
    code_.insert(code_.cbegin() + index, value);
}

void CodeChunk::replace(size_t index, const std::string& value) {
    assert(index < code_.size() && "indexing CodeChunk should always be valid");
    code_[index] = value;
}

std::string CodeChunk::toStr() const {
    std::string codestr{};
    for (const auto& instr : code_) {
        codestr += instr;
    }

    return codestr;
}

std::string CodeChunk::toStrTo(size_t lineNumber) const {
    if (lineNumber > code_.size()) {
        return this->toStr();
    }

    std::string codestr{};
    for (size_t i = 0; i < lineNumber; ++i) {
        const auto& instr = code_[i];
        codestr += instr;
    }

    return codestr;
}

std::string CodeChunk::toStrFrom(size_t lineNumber) const {
    std::string codestr{};
    for (size_t i = lineNumber; i < code_.size(); ++i) {
        const auto& instr = code_[i];
        codestr += instr;
    }

    return codestr;
}

std::any Compiler::visitProgram(qasm3Parser::ProgramContext* ctx) {
    // do the pass to count numQubits;
    firstPass(ctx);

    code_.add(Generator::emitDepsHeaders());
    code_.add("using namespace qasc;\n");
    visitChildren(ctx);
    if (!startedMain_) {
        tryRegisterMainFn();
    }
    code_.insert(firstLineInMainIdx_, Generator::emitInputs(inputs_));

    // check if user has defined main themselves, if so, return early
    if (std::find_if(definedFuncs_.begin(), definedFuncs_.end(),
                     [](const auto& fnSig) { return "main" == fnSig.fnName; })
        != definedFuncs_.end()) {
        return true;
    }

    if (outputFilename_ != "<noname>") {
        if (!outputs_.empty()) {
            code_.add(Generator::emitOutput(outputFilename_, outputs_));
        } else if (!potentialOutputs_.empty()) {
            code_.add(
                Generator::emitOutput(outputFilename_, potentialOutputs_));
        }
    }

    if (isQbackendInit_ && !callingFromInclude_) {
        code_.add(Generator::emitQuestFree());
    }
    tryFinalizingMainFn();
    return true;
}

std::any Compiler::visitVersion(qasm3Parser::VersionContext* ctx) {
    return true;
}

std::any Compiler::visitStatement(qasm3Parser::StatementContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitAnnotation(qasm3Parser::AnnotationContext* ctx) {
    throw std::runtime_error(
        errorMessage(ctx, "annotation directives are not supported."));
}

std::any Compiler::visitScope(qasm3Parser::ScopeContext* ctx) {
    scopeLvl_++;
    code_.add(Generator::emitLeftBracket());

    if (callingFromMainFn_ && !includeCodeForMainFn_.empty()) {
        auto codeFromIncludesVec = getCodeFromIncludes();
        std::string codeFromIncludes;
        for (const auto& code : codeFromIncludesVec) {
            codeFromIncludes += code;
        }
        code_.add(std::move(codeFromIncludes));
    }

    visitChildren(ctx);
    code_.add(Generator::emitRightBracket());
    scopeLvl_--;
    return true;
}

std::any Compiler::visitPragma(qasm3Parser::PragmaContext* ctx) {
    throw std::runtime_error(
        errorMessage(ctx, "pragma directives are not supported."));
}

std::any Compiler::visitStatementOrScope(
    qasm3Parser::StatementOrScopeContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitCalibrationGrammarStatement(
    qasm3Parser::CalibrationGrammarStatementContext* ctx) {
    throw std::runtime_error(
        errorMessage(ctx, "calibration grammar statements are not supported."));
}

void Compiler::getDefinedFuncsFromInclude(
    const std::vector<FuncSignature>& otherDefinedFuncs) {
    for (auto& fnSignature : otherDefinedFuncs) {
        definedFuncs_.push_back(fnSignature);
    }
}

std::any Compiler::visitIncludeStatement(
    qasm3Parser::IncludeStatementContext* ctx) {
    auto includeFile = ctx->StringLiteral()->getText();
    includeFile = std::string(includeFile.begin() + 1, includeFile.end() - 1);
    if (includeFile == "stdgates.inc" || includeFile == "qelib1.inc") {
        Utils::print(
            std::format("{}", warningMessage(ctx, "ignoring include file.")));
        return true;
    }
    auto includePath = includeFile;
    std::ifstream stream;

    if (inputFilename_ != "<noname>") {
        auto index = inputFilename_.find_last_of('/');
        std::string srcPath = "";
        if (index != std::string::npos) {
            srcPath = std::string(inputFilename_.begin(),
                                  inputFilename_.begin() + index + 1);
        }
        includePath = srcPath + includePath;
    }

    stream.open(includePath);
    antlr4::ANTLRInputStream input(stream);
    antlr4cpp::qasm3Lexer lexer(&input);
    antlr4::CommonTokenStream tokens(&lexer);
    qasm3Parser parser(&tokens);
    stream.close();
    qasm3Parser::ProgramContext* tree = parser.program();

    tempFilename_ = currentFilename_;
    currentFilename_ = includeFile;

    Compiler compiler(true);
    compiler.visitProgram(tree);

    if (compiler.totalNumQubits() != "0") {
        totalNumQubits_ += " + " + compiler.totalNumQubits();
    }
    currentFilename_ = tempFilename_;

    getDefinedFuncsFromInclude(compiler.definedFuncs_);

    std::string headerDef = includeFile;
    std::replace(headerDef.begin(), headerDef.end(), '.', '_');
    std::transform(headerDef.begin(), headerDef.end(), headerDef.begin(),
                   ::toupper);

    // if we have nested includes (A includes B which includes C)
    // remember to take everything
    // NOTE: but if we have duplicated includes with expressions
    // that require main it will cause errors. But that's on user
    for (const auto& codeFromInclude : compiler.includeCodeForMainFn_) {
        includeCodeForMainFn_.emplace_back(codeFromInclude);
    }

    // handle code from include that should be run inside main function
    if (compiler.includeMainLineNumber() != -1) {
        const auto mainLineNumber = compiler.includeMainLineNumber();
        code_.add(Generator::emitIncludeCode(
            headerDef, compiler.getCodeTo(mainLineNumber)));

        // this will be added to main later on when main is registered
        // we defer it to first handle all include files
        includeCodeForMainFn_.emplace_back(
            compiler.getCodeFrom(mainLineNumber));
    } else {
        code_.add(Generator::emitIncludeCode(headerDef, compiler.getCode()));
    }

    return true;
}

std::any Compiler::visitBreakStatement(
    qasm3Parser::BreakStatementContext* ctx) {
    code_.add(Generator::emitBreakStmt());
    return true;
}

std::any Compiler::visitContinueStatement(
    qasm3Parser::ContinueStatementContext* ctx) {
    code_.add(Generator::emitContinueStmt());
    return true;
}

std::any Compiler::visitEndStatement(qasm3Parser::EndStatementContext* ctx) {
    tryRegisterMainFn();
    code_.add(Generator::emitEndStmt());
    return true;
}

std::any Compiler::visitForStatement(qasm3Parser::ForStatementContext* ctx) {
    tryRegisterMainFn();

    auto id = ctx->Identifier()->getText();
    if (ctx->rangeExpression()) {
        auto start = ctx->rangeExpression()->expression(0)->getText();
        start = Utils::replacePowerOp(start);
        start = Utils::replaceAllCastExpr(start);

        auto end = ctx->rangeExpression()->expression(1)->getText();
        end = Utils::replacePowerOp(end);
        end = Utils::replaceAllCastExpr(end);

        std::string increment = "1";
        if (ctx->rangeExpression()->expression().size() == 3) {
            increment = ctx->rangeExpression()->expression(2)->getText();
            increment = Utils::replacePowerOp(increment);
            increment = Utils::replaceAllCastExpr(increment);
            // [x:y:z] z is actually end, and y is increment
            std::swap(end, increment);
        }

        code_.add(Generator::emitForStmt(id, start, end, increment));
    } else if (ctx->setExpression()) {
        code_.add(Generator::emitForStmt(id, ctx->setExpression()->getText()));
    } else if (ctx->expression()) {
        code_.add(Generator::emitForStmt(id, ctx->expression()->getText()));
    } else {
        throw std::runtime_error(errorMessage(
            ctx, "for loop supports only range and set expressions."));
    }

    ctx->statementOrScope()->accept(this);
    return true;
}

std::any Compiler::visitIfStatement(qasm3Parser::IfStatementContext* ctx) {
    tryRegisterMainFn();

    if (!ctx->expression()) {
        throw std::runtime_error(
            errorMessage(ctx, "missing expressions in if statement."));
    }

    auto expr = ctx->expression()->getText();
    expr = Utils::replacePowerOp(expr);
    expr = Utils::replaceAllCastExpr(expr);

    code_.add(Generator::emitIfStmt(expr));
    ctx->if_body->accept(this);

    if (ctx->ELSE()) {
        code_.add(Generator::emitElseStmt());
        ctx->else_body->accept(this);
    }

    return true;
}

std::any Compiler::visitReturnStatement(
    qasm3Parser::ReturnStatementContext* ctx) {
    tryRegisterMainFn();

    if (ctx->measureExpression()) {
        code_.add("return ");
        ctx->measureExpression()->accept(this);
        return true;
    }
    if (!ctx->expression()) {
        code_.add(Generator::emitReturnStmt(""));
    } else {
        code_.add(Generator::emitReturnStmt(ctx->expression()->getText()));
    }
    return true;
}

std::any Compiler::visitWhileStatement(
    qasm3Parser::WhileStatementContext* ctx) {
    tryRegisterMainFn();

    auto expr = ctx->expression()->getText();
    expr = Utils::replacePowerOp(expr);
    expr = Utils::replaceAllCastExpr(expr);

    code_.add(Generator::emitWhileStmt(expr));
    ctx->statementOrScope()->accept(this);
    return true;
}

std::any Compiler::visitSwitchStatement(
    qasm3Parser::SwitchStatementContext* ctx) {
    tryRegisterMainFn();

    auto expr = ctx->expression()->getText();
    expr = Utils::replacePowerOp(expr);
    expr = Utils::replaceAllCastExpr(expr);
    code_.add(Generator::emitSwitchStmt(expr));
    code_.add(Generator::emitLeftBracket());
    for (auto c : ctx->switchCaseItem()) {
        c->accept(this);
    }
    code_.add(Generator::emitRightBracket());
    return true;
}

std::any Compiler::visitSwitchCaseItem(
    qasm3Parser::SwitchCaseItemContext* ctx) {
    if (ctx->CASE()) {
        for (auto& e : ctx->expressionList()->expression()) {
            auto expr = e->getText();
            expr = Utils::replacePowerOp(expr);
            expr = Utils::replaceAllCastExpr(expr);

            code_.add(Generator::emitCaseStmt(expr));
            code_.add(Generator::emitLeftBracket());
            ctx->scope()->accept(this);
            code_.add(Generator::emitBreakStmt());
            code_.add(Generator::emitRightBracket());
        }
        return true;
    } else if (ctx->DEFAULT()) {
        code_.add(Generator::emitDefaultStmt());
    }

    code_.add(Generator::emitLeftBracket());
    ctx->scope()->accept(this);
    code_.add(Generator::emitBreakStmt());
    code_.add(Generator::emitRightBracket());

    return true;
}

std::any Compiler::visitBarrierStatement(
    qasm3Parser::BarrierStatementContext* ctx) {
    Utils::print(
        std::format("{}", warningMessage(ctx, "ignoring barrier statement.")));
    return true;
}

std::any Compiler::visitBoxStatement(qasm3Parser::BoxStatementContext* ctx) {
    tryRegisterMainFn();

    ctx->scope()->accept(this);
    return true;
}

std::any Compiler::visitDelayStatement(
    qasm3Parser::DelayStatementContext* ctx) {
    warningMessage(ctx, "ignoring delay statement.");
    return true;
}

std::any Compiler::visitGateCallStatement(
    qasm3Parser::GateCallStatementContext* ctx) {
    if (ctx->gateOperandList()) {
        for (auto& operand : ctx->gateOperandList()->gateOperand()) {
            if (operand->HardwareQubit()) {
                throw std::runtime_error(errorMessage(
                    ctx, "addressing hardware qubits is not supported."));
            }
        }
    }

    tryRegisterMainFn();

    if (callingFromDefStmt_) {
        definedFuncs_.back().callsGates = true;
    } else {
        initQuantumBackend();
    }

    std::string statement = ctx->getText();
    if (statement.find("gphase") != statement.npos) {
        statement = Utils::processExpression(statement);
        code_.add(std::move(statement) + "\n");
        return true;
    }

    auto gateName{ ctx->Identifier()->getText() };
    auto gatePower{ 1 };
    std::string gatePowerStr{ "" };
    handleGateModifier(ctx->gateModifier(), gateName, gatePower, gatePowerStr);

    if (applyingInverse_) {
        gateName = "inv_" + gateName;
        applyingInverse_ = false;
    }

    std::string gateCall = gateName + "(";

    if (ctx->expressionList()) {
        std::string params = ctx->expressionList()->getText();
        params = Utils::processExpression(params);
        gateCall += params + ", ";
    }

    if (ctx->gateOperandList()) {
        std::string qubits = ctx->gateOperandList()->getText();
        qubits = Utils::processExpression(qubits);
        qubits = Utils::replaceQuregSlice(qubits);
        gateCall += qubits + ");\n";
    }

    if (gatePower > 1) {
        code_.add(
            std::format("for (auto gatePowerIdx = 0; gatePowerIdx < {}; "
                        "++gatePowerIdx)",
                        gatePowerStr));
        code_.add(" {\n");
    }
    code_.add(std::move(gateCall));
    if (gatePower > 1) {
        code_.add("\n}\n");
    }

    return true;
}

std::any Compiler::visitMeasureArrowAssignmentStatement(
    qasm3Parser::MeasureArrowAssignmentStatementContext* ctx) {
    tryRegisterMainFn();

    if (callingFromDefStmt_) {
        definedFuncs_.back().callsGates = true;
    } else {
        initQuantumBackend();
    }

    auto varName = ctx->indexedIdentifier()->getText();
    varName = Utils::replaceArrayCommaExpr(varName);
    currentAssignmentName_ = varName;
    if (!ctx->indexedIdentifier()->indexOperator().empty()) {
        if (!ctx->indexedIdentifier()
                 ->indexOperator(0)
                 ->rangeExpression()
                 .empty()) {
            processingSlice_ = true;
            if (ctx->indexedIdentifier()->indexOperator().size() > 1
                || !ctx->indexedIdentifier()
                        ->indexOperator(0)
                        ->COMMA()
                        .empty()) {
                throw std::runtime_error(errorMessage(
                    ctx, std::format("currently only 1D slices are "
                                     "allowed. From {}",
                                     ctx->getText())));
            }
            code_.add(
                std::format("setSlice({}, ",
                            ctx->indexedIdentifier()->Identifier()->getText()));
        } else if (ctx->indexedIdentifier()
                       ->indexOperator(0)
                       ->setExpression()) {
            throw std::runtime_error(
                errorMessage(ctx,
                             "set indexing expression in quantum measurements "
                             "are not supported."));
        } else {
            code_.add(Generator::emitAssignmentFront(varName, "=")
                      + "one_qubit_");
        }
    } else {
        code_.add(Generator::emitAssignmentFront(varName, "="));
    }
    ctx->measureExpression()->accept(this);
    if (processingSlice_) {
        // remove ;\n and add , instead
        code_.pop();
        code_.add(", ");

        ctx->indexedIdentifier()->indexOperator(0)->accept(this);
        if (ctx->SEMICOLON()) {
            code_.add(";\n");
        }
        processingSlice_ = false;
    }
    currentAssignmentName_ = "";
    return true;
}

std::any Compiler::visitResetStatement(
    qasm3Parser::ResetStatementContext* ctx) {
    tryRegisterMainFn();

    if (callingFromDefStmt_) {
        definedFuncs_.back().callsGates = true;
    } else {
        initQuantumBackend();
    }

    std::string qubits = ctx->gateOperand()->getText();
    qubits = Utils::processExpression(qubits);
    qubits = Utils::replaceQuregSlice(qubits);
    code_.add("reset(" + qubits + ");\n");
    return true;
}

std::any Compiler::visitAliasDeclarationStatement(
    qasm3Parser::AliasDeclarationStatementContext* ctx) {
    callingFromDeclaration_ = true;
    callingFromAliasDeclaration_ = true;
    const auto id = ctx->Identifier()->toString();
    if (!ctx->aliasExpression()) {
        throw std::runtime_error(
            errorMessage(ctx, "invalid alias expressions."));
    }

    if (!ctx->aliasExpression()->DOUBLE_PLUS().empty()) {
        throw std::runtime_error(errorMessage(
            ctx, "concatenated alias expressions are not supported."));
    }

    code_.add(std::format("auto {} = ", id));
    for (auto& expr : ctx->aliasExpression()->expression()) {
        expr->accept(this);
    }
    if (ctx->SEMICOLON()) {
        code_.add(";\n");
    }

    callingFromAliasDeclaration_ = false;
    callingFromDeclaration_ = false;
    return true;
}

std::any Compiler::visitClassicalDeclarationStatement(
    qasm3Parser::ClassicalDeclarationStatementContext* ctx) {
    callingFromDeclaration_ = true;
    if (ctx->scalarType()) {
        Compiler::generateScalarDeclaration<
            qasm3Parser::ClassicalDeclarationStatementContext>(ctx);
    } else if (ctx->arrayType()) {
        Compiler::generateArrayDeclaration<
            qasm3Parser::ClassicalDeclarationStatementContext>(ctx);
    } else {
        throw std::runtime_error(errorMessage(
            ctx, std::format("error in classical declaration in {}.",
                             ctx->getText())));
    }

    callingFromDeclaration_ = false;
    return true;
}

std::any Compiler::visitConstDeclarationStatement(
    qasm3Parser::ConstDeclarationStatementContext* ctx) {
    callingFromDeclaration_ = true;
    code_.add(Generator::emitConstMod(startedMain_));
    if (ctx->scalarType()) {
        Compiler::generateScalarDeclaration<
            qasm3Parser::ConstDeclarationStatementContext>(ctx);
    } else {
        throw std::runtime_error(errorMessage(
            ctx,
            std::format("error in const declaration in {}.", ctx->getText())));
    }

    callingFromDeclaration_ = false;
    return true;
}

std::any Compiler::visitIoDeclarationStatement(
    qasm3Parser::IoDeclarationStatementContext* ctx) {
    callingFromDeclaration_ = true;
    if (ctx->scalarType()) {
        auto type{ extractScalarType(ctx, ctx->scalarType()->getText()) };
        if (type == "float") {
            type = "double";
        }

        const auto id{ ctx->Identifier()->getText() };

        if (type != "double" && type != "std::complex<double>") {
            std::string bitArraySize = "32";
            std::string isAngle = "";

            if (type.find(", true>") != type.npos) {
                isAngle = ", true";
            }

            if (ctx->scalarType()->BIT() || ctx->scalarType()->BOOL()) {
                bitArraySize = "1";
            }

            if (ctx->scalarType()->designator()
                && ctx->scalarType()->designator()->expression()) {
                bitArraySize
                    = ctx->scalarType()->designator()->expression()->getText();
            }

            auto commaPos = type.find(',');
            type = type.substr(0, commaPos + 1);
            type += std::format(" {}{}>", bitArraySize, isAngle);
        }

        if (ctx->OUTPUT()) {
            code_.add(Generator::emitIoDeclStmt(type, id, ""));
            outputs_.emplace_back(Output{ type, id, 0 });
        } else if (ctx->INPUT()) {
            inputs_.emplace_back(Input{ type, id, 0 });
        } else {
            throw std::runtime_error(errorMessage(
                ctx,
                std::format("error in IO declaration in {}.", ctx->getText())));
        }
    } else if (ctx->arrayType()) {
        const int numDim
            = ctx->arrayType()->expressionList()->expression().size();
        std::string type
            = extractScalarType(ctx, ctx->arrayType()->scalarType()->getText());
        if (type == "float") {
            type = "double";
        }

        if (type != "double" && type != "std::complex<double>") {
            std::string bitArraySize = "32";
            std::string isAngle = "";

            if (type.find(", true>") != type.npos) {
                isAngle = ", true";
            }

            if (ctx->arrayType()->scalarType()->BIT()
                || ctx->arrayType()->scalarType()->BOOL()) {
                bitArraySize = "1";
            }

            if (ctx->arrayType()->scalarType()->designator()
                && ctx->arrayType()->scalarType()->designator()->expression()) {
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

        const auto id{ ctx->Identifier()->getText() };
        std::string defaultIntializer{ "" };
        for (int i = 0; i < numDim; ++i) {
            auto expr
                = ctx->arrayType()->expressionList()->expression(i)->getText();
            std::string sndArg = ", 0)";    // default 0 initialization
            if (numDim - i - 1 != 0) {
                sndArg
                    = ", " + Utils::createNDimVectorStr(numDim - i - 1, type);
            }
            defaultIntializer += "(" + expr + sndArg;
        }

        for (int i = 1; i < numDim; ++i) {
            defaultIntializer += ")";
        }

        if (ctx->OUTPUT()) {
            code_.add(Generator::emitIoDeclStmt(
                Generator::emitNdArray(numDim, type), id, defaultIntializer));
            outputs_.emplace_back(Output{ type, id, numDim });
        } else if (ctx->INPUT()) {
            if (numDim > 1) {
                throw std::runtime_error(errorMessage(
                    ctx, "multidimensional inputs are not supported."));
            }
            inputs_.emplace_back(Input{ type, id, numDim });
        } else {
            throw std::runtime_error(errorMessage(
                ctx,
                std::format("error in IO declaration in {}.", ctx->getText())));
        }
    } else {
        throw std::runtime_error(errorMessage(
            ctx,
            std::format("error in IO declaration in {}.", ctx->getText())));
    }

    callingFromDeclaration_ = false;
    return true;
}

std::any Compiler::visitOldStyleDeclarationStatement(
    qasm3Parser::OldStyleDeclarationStatementContext* ctx) {
    tryRegisterMainFn();

    const auto regName = ctx->Identifier()->getText();
    if (!ctx->designator()) {
        throw std::runtime_error(errorMessage(
            ctx, std::format("did not specify register size in {}.",
                             ctx->getText())));
    }
    auto expression = ctx->designator()->expression()->getText();
    expression = Utils::replacePowerOp(expression);
    expression = Utils::replaceAllCastExpr(expression);

    if (ctx->CREG()) {
        const auto defaultInitializer = "(0)";
        const auto type = std::format("QasmType<unsigned int, {}>", expression);
        code_.add(Generator::emitClassicalDeclStmt(type, regName,
                                                   defaultInitializer));

        if (scopeLvl_ == 0) {
            potentialOutputs_.emplace_back(Output{ type, regName, 0 });
        }
    } else if (ctx->QREG()) {
        code_.add(
            std::format("auto {} = QuregSlice({});\n", regName, expression));
        if (regName == lastQuregName_) {
            initQuantumBackend();
        }

    } else {
        throw std::runtime_error(errorMessage(
            ctx, std::format("error in register declaration in {}.",
                             ctx->getText())));
    }
    return true;
}

std::any Compiler::visitQuantumDeclarationStatement(
    qasm3Parser::QuantumDeclarationStatementContext* ctx) {
    if (!callingFromDefStmt_) {
        tryRegisterMainFn();
    }

    const auto qregName = ctx->Identifier()->getText();
    std::string expression = "1";

    if (ctx->qubitType()->designator()
        && ctx->qubitType()->designator()->expression()) {
        expression = ctx->qubitType()->designator()->expression()->getText();
        expression = Utils::replacePowerOp(expression);
        expression = Utils::replaceAllCastExpr(expression);
    }

    if (expression.size() > 0
        && (expression[0] == '-' || expression[0] == '0')) {
        throw std::runtime_error(
            errorMessage(ctx,
                         "invalid qubit declaration. Number of qubits should "
                         "be positive."));
    }
    code_.add(std::format("auto {} = QuregSlice({});\n", qregName, expression));

    if (qregName == lastQuregName_) {
        initQuantumBackend();
    }

    return true;
}

std::any Compiler::visitDefStatement(qasm3Parser::DefStatementContext* ctx) {
    callingFromDefStmt_ = true;
    std::string returnType{ "" };

    const auto codeSizeOnDefEnter = code_.size();

    if (!ctx->returnSignature()) {
        returnType = "void";
    } else {
        returnType = extractScalarType(
            ctx, ctx->returnSignature()->scalarType()->getText());
    }

    const auto fnName = ctx->Identifier()->getText();

    if (fnName == "main") {
        callingFromMainFn_ = true;
        startedMain_ = true;
        returnType = "int";
    }

    definedFuncs_.emplace_back(fnName, returnType);

    std::string args{ "" };
    if (ctx->argumentDefinitionList()) {
        args = std::any_cast<std::string>(
            ctx->argumentDefinitionList()->accept(this));
    }

    if (fnName == "main") {
        if (args != "") {
            args.append(", ");
        }
        args.append("int argc, char* argv[]");
    }

    code_.add(Generator::emitFuncDefStmt(returnType, fnName, args));

    if (fnName == "main") {
        firstLineInMainIdx_ = code_.size() + 1;
    }

    ctx->scope()->accept(this);

    auto& fnSignature = definedFuncs_.back();
    size_t numArrArgs = 0;
    bool hasArrArg = false;
    bool hasQuregArg = false;
    for (auto& argType : fnSignature.argTypes) {
        if (argType.find("std::vector") != std::string::npos) {
            ++numArrArgs;
        }
    }

    if (numArrArgs > 0) {
        hasArrArg = true;
    }

    if (std::find(fnSignature.argTypes.begin(), fnSignature.argTypes.end(),
                  "QuregSlice & ")
        != fnSignature.argTypes.end()) {
        hasQuregArg = true;
    }

    std::string templateExpr{ "" };
    if (hasQuregArg && hasArrArg) {
        templateExpr += "template <QasmQuregT T";
        for (size_t i = 0; i < numArrArgs; ++i) {
            templateExpr += std::format(", typename VecT{}", i);
        }
        templateExpr += ">\n";
    } else if (hasQuregArg) {
        templateExpr = "template <QasmQuregT T>\n";
    } else if (hasArrArg) {
        templateExpr = "template <typename VecT0 ";
        for (size_t i = 1; i < numArrArgs; ++i) {
            templateExpr += std::format(", typename VecT{}", i);
        }
        templateExpr += ">\n";
    }
    code_.insert(codeSizeOnDefEnter, std::move(templateExpr));

    if (fnName == "main") {
        code_.pop();    // remove } from scope body
        if (code_.last().starts_with("return")) {
            code_.pop();    // remove last return
        }

        if (outputFilename_ != "<noname>") {
            if (!outputs_.empty()) {
                code_.add(Generator::emitOutput(outputFilename_, outputs_));
            } else if (!potentialOutputs_.empty()) {
                code_.add(
                    Generator::emitOutput(outputFilename_, potentialOutputs_));
            }
        }

        if (isQbackendInit_) {
            code_.add(Generator::emitQuestFree());
        }
        code_.add("return 0;\n");
        code_.add("}\n");
        callingFromMainFn_ = false;
    }

    const auto codeSizeOnDefExit = code_.size();

    // user has used an expression that required registering c++ main function.
    // defining function inside c++ main function body is invalid, so
    // the function definition has to be moved before the main function.
    if (startedMain_ && codeSizeOnDefEnter > firstLineInMainIdx_) {
        int i = codeSizeOnDefExit - codeSizeOnDefEnter;
        std::vector<std::string> fullDefStmt(i);
        while (i > 0) {
            --i;
            fullDefStmt.emplace_back(code_.last());
            code_.pop();
        }
        std::string fullDefStmtStr{ "" };
        std::reverse(fullDefStmt.begin(), fullDefStmt.end());
        for (auto& s : fullDefStmt) {
            fullDefStmtStr += s;
        }
        code_.insert(firstLineInMainIdx_ - 1, std::move(fullDefStmtStr));
        ++firstLineInMainIdx_;
    }

    callingFromDefStmt_ = false;

    return true;
}

std::any Compiler::visitExternStatement(
    qasm3Parser::ExternStatementContext* ctx) {
    throw std::runtime_error(
        errorMessage(ctx, "extern statement is currently not supported."));
}

std::any Compiler::visitGateStatement(qasm3Parser::GateStatementContext* ctx) {
    const auto codeSizeOnGateDefEnter = code_.size();
    callingFromGateDeclaration_ = true;
    const auto returnType = "void";
    const auto gateName = ctx->Identifier()->getText();

    std::string args;
    int numParams = 0;
    if (ctx->params) {
        auto paramsArgs = "double " + ctx->params->getText();
        ++numParams;
        size_t pos = paramsArgs.find(',');
        const std::string paramsArgType = " double ";

        // add type after each comma
        while (pos != std::string::npos) {
            ++numParams;
            paramsArgs.insert(pos + 1, paramsArgType);
            pos = paramsArgs.find(',', pos + 1 + paramsArgType.length());
        }

        args += paramsArgs;
    }

    if (ctx->qubits) {
        if (numParams > 0) {
            args += ",";
        }
        const std::string qubitArgType = " T ";
        auto qubitArgs = qubitArgType + ctx->qubits->getText();
        size_t pos = qubitArgs.find(',');

        // add type after each comma
        while (pos != std::string::npos) {
            qubitArgs.insert(pos + 1, qubitArgType);
            pos = qubitArgs.find(',', pos + 1 + qubitArgType.length());
        }
        args += qubitArgs;
    }

    code_.add("template <QasmQuregT T>\n");
    code_.add(Generator::emitFuncDefStmt(returnType, gateName, args));

    ctx->scope()->accept(this);

    const auto codeSizeOnGateDefExit = code_.size();
    // user has used an expression that required registering c++ main function.
    // defining function inside c++ main function body is invalid, so
    // the function definition has to be moved before the main function.
    if (startedMain_ && codeSizeOnGateDefEnter > firstLineInMainIdx_) {
        int i = codeSizeOnGateDefExit - codeSizeOnGateDefEnter;
        std::vector<std::string> fullDefStmt(i);
        while (i > 0) {
            --i;
            fullDefStmt.emplace_back(code_.last());
            code_.pop();
        }
        std::string fullDefStmtStr{ "" };
        std::reverse(fullDefStmt.begin(), fullDefStmt.end());
        for (auto& s : fullDefStmt) {
            fullDefStmtStr += s;
        }
        code_.insert(firstLineInMainIdx_ - 1, std::move(fullDefStmtStr));
        ++firstLineInMainIdx_;
    }

    callingFromGateDeclaration_ = false;
    return true;
}

std::any Compiler::visitAssignmentStatement(
    qasm3Parser::AssignmentStatementContext* ctx) {
    if (!callingFromDefStmt_ && !callingFromGateDeclaration_) {
        tryRegisterMainFn();
    }

    if (ctx->measureExpression()) {
        if (callingFromDefStmt_) {
            definedFuncs_.back().callsGates = true;
        } else {
            initQuantumBackend();
        }

        auto varName{ ctx->getText().substr(0, ctx->getText().find("=")) };
        if (currentAssignmentName_ == "") {
            currentAssignmentName_ = varName;
        }

        // if var is of form: name[x, y, z] output: name[x][y][z]
        varName = Utils::replaceArrayCommaExpr(varName);

        // if var is of form name[x:y] or name[x:y:z]
        // output: setSlice(name, rhs, x, y, z)

        if (!ctx->indexedIdentifier()->indexOperator().empty()) {
            if (!ctx->indexedIdentifier()
                     ->indexOperator(0)
                     ->rangeExpression()
                     .empty()) {
                processingSlice_ = true;
                if (ctx->indexedIdentifier()->indexOperator().size() > 1
                    || !ctx->indexedIdentifier()
                            ->indexOperator(0)
                            ->COMMA()
                            .empty()) {
                    throw std::runtime_error(errorMessage(
                        ctx, std::format("currently only 1D slices are "
                                         "allowed. From {}",
                                         ctx->getText())));
                }
                code_.add(std::format(
                    "setSlice({}, ",
                    ctx->indexedIdentifier()->Identifier()->getText()));
            } else if (ctx->indexedIdentifier()
                           ->indexOperator(0)
                           ->setExpression()) {
                throw std::runtime_error(errorMessage(
                    ctx,
                    "set indexing expression in quantum measurements "
                    "are not supported."));
            } else {
                code_.add(Generator::emitAssignmentFront(varName, "=")
                          + "one_qubit_");
            }

        } else {
            code_.add(Generator::emitAssignmentFront(varName, "="));
        }
        ctx->measureExpression()->accept(this);
        if (processingSlice_) {
            // remove ;\n and add , instead
            code_.pop();
            code_.add(", ");

            ctx->indexedIdentifier()->indexOperator(0)->accept(this);
            if (ctx->SEMICOLON()) {
                code_.add(";\n");
            }
            processingSlice_ = false;
        }
        currentAssignmentName_ = "";
        return true;
    }

    auto varName = ctx->indexedIdentifier()->getText();
    if (currentAssignmentName_ == "") {
        currentAssignmentName_ = varName;
    }

    // if var is of form: name[x, y, z] output: name[x][y][z]
    varName = Utils::replaceArrayCommaExpr(varName);

    if (ctx->EQUALS()) {
        code_.add(Generator::emitAssignmentFront(varName, "="));
    } else {
        const auto op = ctx->CompoundAssignmentOperator()->getText();
        code_.add(Generator::emitAssignmentFront(varName, op));
    }

    if (!ctx->indexedIdentifier()->indexOperator().empty()) {
        if (!ctx->indexedIdentifier()
                 ->indexOperator(0)
                 ->rangeExpression()
                 .empty()) {
            // we don't need = operator
            code_.pop();

            processingSlice_ = true;
            if (ctx->indexedIdentifier()->indexOperator().size() > 1
                || !ctx->indexedIdentifier()
                        ->indexOperator(0)
                        ->COMMA()
                        .empty()) {
                throw std::runtime_error(errorMessage(
                    ctx, std::format("currently only 1D slices are "
                                     "allowed. From {}",
                                     ctx->getText())));
            }
            code_.add(
                std::format("setSlice({}, ",
                            ctx->indexedIdentifier()->Identifier()->getText()));

            processingRHSAsSlice_ = true;
        }
    }

    callingFromAssignment_ = true;
    if (ctx->expression()) {
        auto expr = ctx->expression()->getText();
        expr = Utils::processExpression(expr);
        code_.add(std::move(expr));
    }

    callingFromAssignment_ = false;

    if (processingSlice_) {
        code_.add(", ");
        ctx->indexedIdentifier()->indexOperator(0)->accept(this);
        if (ctx->SEMICOLON()) {
            code_.add(";\n");
        }
        processingSlice_ = false;
        currentAssignmentName_ = "";
        return true;
    }

    code_.add(Generator::emitAssignmentBack());
    currentAssignmentName_ = "";
    return true;
}

std::any Compiler::visitExpressionStatement(
    qasm3Parser::ExpressionStatementContext* ctx) {
    tryRegisterMainFn();
    auto res = ctx->expression()->accept(this);

    if (ctx->SEMICOLON()) {
        code_.add(";\n");
    }
    return true;
}

std::any Compiler::visitCalStatement(qasm3Parser::CalStatementContext* ctx) {
    throw std::runtime_error(
        errorMessage(ctx, "cal statement is currently not supported."));
}

std::any Compiler::visitDefcalStatement(
    qasm3Parser::DefcalStatementContext* ctx) {
    throw std::runtime_error(
        errorMessage(ctx, "defcal statement is currently not supported."));
}

std::any Compiler::visitBitwiseXorExpression(
    qasm3Parser::BitwiseXorExpressionContext* ctx) {
    tryRegisterMainFn();
    if (ctx->expression().size() != 2) {
        throw std::runtime_error(errorMessage(
            ctx,
            std::format("invalid number of operands in {}", ctx->getText())));
    }

    // LHS
    ctx->expression(0)->accept(this);
    if (!ctx->CARET()) {
        throw std::runtime_error(errorMessage(
            ctx, std::format("invalid operator in Bitwise Xor expression, {}.",
                             ctx->getText())));
    }
    code_.add(Generator::emitXorOp());

    // RHS
    ctx->expression(1)->accept(this);
    return OpType::QASC_BINARY_EXPRESSION;
}

std::any Compiler::visitAdditiveExpression(
    qasm3Parser::AdditiveExpressionContext* ctx) {
    tryRegisterMainFn();
    if (ctx->expression().size() != 2) {
        throw std::runtime_error(errorMessage(
            ctx,
            std::format("invalid number of operands in {}", ctx->getText())));
    }

    // LHS
    ctx->expression(0)->accept(this);

    if (ctx->PLUS()) {
        code_.add(Generator::emitPlusOp());
    } else if (ctx->MINUS()) {
        code_.add(Generator::emitMinusOp());
    } else {
        throw std::runtime_error(errorMessage(
            ctx,
            std::format("invalid additive operator in {}.", ctx->getText())));
    }

    // RHS
    ctx->expression(1)->accept(this);
    return OpType::QASC_BINARY_EXPRESSION;
}

std::any Compiler::visitDurationofExpression(
    qasm3Parser::DurationofExpressionContext* ctx) {
    throw std::runtime_error(
        errorMessage(ctx, "durationof expression is not supported."));

    return true;
}

std::any Compiler::visitParenthesisExpression(
    qasm3Parser::ParenthesisExpressionContext* ctx) {
    code_.add("(");
    visitChildren(ctx);
    code_.add(")");
    return true;
}

std::any Compiler::visitComparisonExpression(
    qasm3Parser::ComparisonExpressionContext* ctx) {
    tryRegisterMainFn();
    if (ctx->expression().size() != 2) {
        throw std::runtime_error(errorMessage(
            ctx,
            std::format("invalid number of operands in {}", ctx->getText())));
    }

    // LHS
    ctx->expression(0)->accept(this);
    code_.add(Generator::emitCompOp(ctx->ComparisonOperator()->toString()));
    // RHS
    ctx->expression(1)->accept(this);
    return OpType::QASC_BINARY_EXPRESSION;
}

std::any Compiler::visitMultiplicativeExpression(
    qasm3Parser::MultiplicativeExpressionContext* ctx) {
    tryRegisterMainFn();
    if (ctx->expression().size() != 2) {
        throw std::runtime_error(errorMessage(
            ctx,
            std::format("invalid number of operands in {}", ctx->getText())));
    }

    ctx->expression(0)->accept(this);
    if (ctx->ASTERISK()) {
        code_.add(Generator::emitMultOp());
    } else if (ctx->SLASH()) {
        code_.add(Generator::emitDivOp());
    } else if (ctx->PERCENT()) {
        code_.add(Generator::emitModOp());
    } else {
        throw std::runtime_error(errorMessage(
            ctx, std::format("invalid operator in {}.", ctx->getText())));
    }
    ctx->expression(1)->accept(this);

    return OpType::QASC_BINARY_EXPRESSION;
}

std::any Compiler::visitLogicalOrExpression(
    qasm3Parser::LogicalOrExpressionContext* ctx) {
    tryRegisterMainFn();
    if (ctx->expression().size() != 2) {
        throw std::runtime_error(errorMessage(
            ctx,
            std::format("invalid number of operands in {}", ctx->getText())));
    }

    // LHS
    ctx->expression(0)->accept(this);
    if (!ctx->DOUBLE_PIPE()) {
        throw std::runtime_error(errorMessage(
            ctx, std::format("invalid operator in Logical Or expression, {}.",
                             ctx->getText())));
    }

    code_.add(Generator::emitDoublePipeOp());

    // RHS
    ctx->expression(1)->accept(this);
    return OpType::QASC_BINARY_EXPRESSION;
}

std::any Compiler::visitCastExpression(
    qasm3Parser::CastExpressionContext* ctx) {
    tryRegisterMainFn();
    if (ctx->scalarType()) {
        const std::string type
            = extractScalarType(ctx, ctx->scalarType()->getText());
        code_.add(Generator::emitCastFront(type));
        ctx->expression()->accept(this);
        code_.add(Generator::emitCastBack());
    } else if (ctx->arrayType()) {
        throw std::runtime_error(
            errorMessage(ctx, "casting array types is not supported"));
    } else {
        throw std::runtime_error(errorMessage(
            ctx, std::format("invalid cast type in cast expression {}",
                             ctx->getText())));
    }

    return true;
}

std::any Compiler::visitPowerExpression(
    qasm3Parser::PowerExpressionContext* ctx) {
    tryRegisterMainFn();
    if (ctx->expression().size() != 2) {
        throw std::runtime_error(errorMessage(
            ctx,
            std::format("invalid number of operands in {}", ctx->getText())));
    }
    auto lhs = ctx->expression(0)->getText();
    lhs = Utils::replacePowerOp(lhs);
    lhs = Utils::replaceAllCastExpr(lhs);
    auto rhs = ctx->expression(1)->getText();
    rhs = Utils::replacePowerOp(rhs);
    rhs = Utils::replaceAllCastExpr(rhs);
    code_.add(Generator::emitPowOp(lhs, rhs));

    return true;
}

std::any Compiler::visitBitwiseOrExpression(
    qasm3Parser::BitwiseOrExpressionContext* ctx) {
    tryRegisterMainFn();
    if (ctx->expression().size() != 2) {
        throw std::runtime_error(errorMessage(
            ctx,
            std::format("invalid number of operands in {}", ctx->getText())));
    }

    // LHS
    ctx->expression(0)->accept(this);
    if (!ctx->PIPE()) {
        throw std::runtime_error(errorMessage(
            ctx, std::format("invalid operator in 'bitwise or' expression, {}.",
                             ctx->getText())));
    }

    code_.add(Generator::emitPipeOp());

    // RHS
    ctx->expression(1)->accept(this);
    return OpType::QASC_BINARY_EXPRESSION;
}

std::any Compiler::visitCallExpression(
    qasm3Parser::CallExpressionContext* ctx) {
    tryRegisterMainFn();
    auto fnName{ ctx->Identifier()->getText() };

    // iterate through expr list
    // find function signature based on function name
    // for each expr:
    //      get the type, transform to valid call and output new expr

    const auto it = std::find_if(
        definedFuncs_.begin(), definedFuncs_.end(),
        [&](const auto& fnSig) { return fnName == fnSig.fnName; });

    if (it == definedFuncs_.end()) {
        throw std::runtime_error(errorMessage(
            ctx, std::format(
                     "could not find the function signature for function: {}.",
                     fnName)));
    }

    const FuncSignature fnSignature = *it;

    if (fnSignature.callsGates) {
        initQuantumBackend();
    }

    std::string expressionList{ "" };
    const auto numArgs = ctx->expressionList()->expression().size();
    if (fnName != "sizeof" && fnSignature.argTypes.size() != numArgs) {
        throw std::runtime_error(
            errorMessage(ctx,
                         "mismatch between number of arguments in function "
                         "call and function definition."));
    }

    for (size_t argIdx = 0; argIdx < numArgs; ++argIdx) {
        auto expr = ctx->expressionList()->expression(argIdx);
        auto exprStr = expr->getText();
        exprStr = Utils::processExpression(exprStr);
        auto argType = fnSignature.argTypes[argIdx];

        if (argType == "QuregSlice & ") {
            // determine quantum slice type
            // 1. it is just q referring to whole slice
            // 2. it is q referring to single qubit (should be the same as
            // whole slice)
            // 3. it is q[idx] -> create 1 qubit slice
            // 4. it is q[a:b] -> create a to b slice
            expressionList += Utils::replaceQuregSlice(exprStr);

        } else if (argType.find("std::vector") != argType.npos) {
            // look for classical slice
            auto newExpr = exprStr;
            newExpr = Utils::replaceSlice(newExpr);

            expressionList += newExpr;
        } else {
            expressionList += exprStr;
        }

        if (argIdx < numArgs - 1) {
            expressionList += ", ";
        }
    }

    if (fnName == "sizeof") {
        fnName = "qasmSizeof";
    }
    code_.add(Generator::emitFuncCallStmt(fnName, expressionList));

    // NOTE: (deprecated) returning false to add ';' in
    // visitExpressionStatement
    return false;
}

std::any Compiler::visitBitshiftExpression(
    qasm3Parser::BitshiftExpressionContext* ctx) {
    tryRegisterMainFn();
    if (ctx->expression().size() != 2) {
        throw std::runtime_error(errorMessage(
            ctx,
            std::format("invalid number of operands in {}", ctx->getText())));
    }

    // LHS
    ctx->expression(0)->accept(this);
    if (!ctx->BitshiftOperator()) {
        throw std::runtime_error(errorMessage(
            ctx, std::format("invalid operator in 'bit shift' expression, {}.",
                             ctx->getText())));
    }

    code_.add(Generator::emitBitshiftOp(ctx->BitshiftOperator()->toString()));

    // RHS
    ctx->expression(1)->accept(this);
    return OpType::QASC_BINARY_EXPRESSION;
}

std::any Compiler::visitBitwiseAndExpression(
    qasm3Parser::BitwiseAndExpressionContext* ctx) {
    tryRegisterMainFn();
    if (ctx->expression().size() != 2) {
        throw std::runtime_error(errorMessage(
            ctx,
            std::format("invalid number of operands in {}", ctx->getText())));
    }

    // LHS
    ctx->expression(0)->accept(this);
    if (!ctx->AMPERSAND()) {
        throw std::runtime_error(errorMessage(
            ctx,
            std::format("invalid operator in 'bitwise and' expression, {}.",
                        ctx->getText())));
    }

    code_.add(Generator::emitAmpersandOp());

    // RHS
    ctx->expression(1)->accept(this);

    return OpType::QASC_BINARY_EXPRESSION;
}

std::any Compiler::visitEqualityExpression(
    qasm3Parser::EqualityExpressionContext* ctx) {
    tryRegisterMainFn();
    if (ctx->expression().size() != 2) {
        throw std::runtime_error(errorMessage(
            ctx,
            std::format("invalid number of operands in {}", ctx->getText())));
    }

    // LHS
    ctx->expression(0)->accept(this);
    if (!ctx->EqualityOperator()) {
        throw std::runtime_error(errorMessage(
            ctx, std::format("invalid operator in 'equality' expression, {}.",
                             ctx->getText())));
    }

    code_.add(Generator::emitEqualityOp(ctx->EqualityOperator()->toString()));

    // RHS
    ctx->expression(1)->accept(this);

    return OpType::QASC_BINARY_EXPRESSION;
}

std::any Compiler::visitLogicalAndExpression(
    qasm3Parser::LogicalAndExpressionContext* ctx) {
    tryRegisterMainFn();
    if (ctx->expression().size() != 2) {
        throw std::runtime_error(errorMessage(
            ctx,
            std::format("invalid number of operands in {}", ctx->getText())));
    }

    // LHS
    ctx->expression(0)->accept(this);
    if (!ctx->DOUBLE_AMPERSAND()) {
        throw std::runtime_error(errorMessage(
            ctx,
            std::format("invalid operator in 'logical and' expression, {}.",
                        ctx->getText())));
    }

    code_.add(Generator::emitDoubleAmpersandOp());

    // RHS
    ctx->expression(1)->accept(this);

    return OpType::QASC_BINARY_EXPRESSION;
}

std::any Compiler::visitIndexExpression(
    qasm3Parser::IndexExpressionContext* ctx) {
    if (callingFromAliasDeclaration_) {
        auto expr = ctx->expression()->getText();
        expr = Utils::replacePowerOp(expr);
        expr = Utils::replaceAllCastExpr(expr);
        code_.add("QuregSlice(" + expr + ", ");
        ctx->indexOperator()->accept(this);
        return true;
    }

    return visitChildren(ctx);
}

std::any Compiler::visitUnaryExpression(
    qasm3Parser::UnaryExpressionContext* ctx) {
    if (ctx->MINUS()) {
        code_.add(Generator::emitMinusUnary());
    } else if (ctx->EXCLAMATION_POINT()) {
        code_.add(Generator::emitExclamationUnary());
    } else if (ctx->TILDE()) {
        code_.add(Generator::emitTildeUnary());
    } else {
        throw std::runtime_error(errorMessage(
            ctx, std::format("invalid operator in {}.", ctx->getText())));
    }
    return visitChildren(ctx);
}

std::any Compiler::visitLiteralExpression(
    qasm3Parser::LiteralExpressionContext* ctx) {
    auto literalStr = ctx->getText();
    literalStr = Utils::processExpression(literalStr);
    code_.add(Generator::emitLiteral(literalStr));
    return true;
}

std::any Compiler::visitAliasExpression(
    qasm3Parser::AliasExpressionContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitDeclarationExpression(
    qasm3Parser::DeclarationExpressionContext* ctx) {
    return true;
}

std::any Compiler::visitMeasureExpression(
    qasm3Parser::MeasureExpressionContext* ctx) {
    tryRegisterMainFn();

    if (callingFromDefStmt_) {
        definedFuncs_.back().callsGates = true;
    } else {
        initQuantumBackend();
    }

    std::string qubits = ctx->gateOperand()->getText();
    qubits = Utils::processExpression(qubits);
    qubits = Utils::replaceQuregSlice(qubits);
    code_.add("measure(" + qubits + ")");
    code_.add(";\n");
    return true;
}

std::any Compiler::visitRangeExpression(
    qasm3Parser::RangeExpressionContext* ctx) {
    std::string parentName
        = ctx->parent->getText().substr(0, ctx->parent->getText().find("["));
    auto start = ctx->expression(0)->getText();
    start = Utils::replacePowerOp(start);
    start = Utils::replaceAllCastExpr(start);
    auto end = ctx->expression(1)->getText();
    end = Utils::replacePowerOp(end);
    end = Utils::replaceAllCastExpr(end);

    std::string increment = "1";
    if (ctx->expression().size() == 3) {
        increment = ctx->expression(2)->getText();
        increment = Utils::replacePowerOp(increment);
        increment = Utils::replaceAllCastExpr(increment);
        // [x:y:z] z is actually end, and y is increment
        std::swap(end, increment);
    }

    code_.add(std::format("getSlice({}, {}, {}, {})", parentName, start, end,
                          increment));
    return true;
}

std::any Compiler::visitSetExpression(qasm3Parser::SetExpressionContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitArrayLiteral(qasm3Parser::ArrayLiteralContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitIndexOperator(qasm3Parser::IndexOperatorContext* ctx) {
    if (!ctx->rangeExpression().empty()) {
        if (ctx->rangeExpression().size() == 1) {
            auto* rangeExpression = ctx->rangeExpression(0);
            auto start = rangeExpression->expression(0)->getText();
            auto end = rangeExpression->expression(1)->getText();
            start = Utils::replacePowerOp(start);
            start = Utils::replaceAllCastExpr(start);
            end = Utils::replacePowerOp(end);
            end = Utils::replaceAllCastExpr(end);

            std::string increment = "1";
            if (rangeExpression->expression().size() == 3) {
                increment = rangeExpression->expression(2)->getText();
                increment = Utils::replacePowerOp(increment);
                increment = Utils::replaceAllCastExpr(increment);
                // [x:y:z] z is actually end, and y is increment
                std::swap(end, increment);
            }

            if (processingRHSAsSlice_ || callingFromAssignment_) {
                processingRHSAsSlice_ = false;
            }

            if (callingFromAliasDeclaration_ || processingSlice_) {
                code_.add(std::format("{}, {}, {})", start, end, increment));
            }
            return true;
        } else {
            throw std::runtime_error(errorMessage(
                ctx,
                std::format("currently only 1D slices are allowed. From {}",
                            ctx->getText())));
        }
    }

    if (ctx->setExpression()) {
        throw std::runtime_error(
            errorMessage(ctx, "array indexing with sets is not supported."));
    }

    // Handling 1 qubit alias / indexing
    if (callingFromAliasDeclaration_) {
        // NOTE: code_.last() == QuregSlice(parentQureg,
        // so we need to add start, end and increment
        visitChildren(ctx);
        code_.add(", ");
        visitChildren(ctx);
        code_.add(", 1)");
        return true;
    }

    std::string idxOperator = ctx->getText();
    idxOperator = Utils::replaceArrayCommaExpr(idxOperator);
    code_.add(std::move(idxOperator));
    return true;
}

std::any Compiler::visitIndexedIdentifier(
    qasm3Parser::IndexedIdentifierContext* ctx) {
    if (callingFromAliasDeclaration_) {
        for (auto& indexOp : ctx->indexOperator()) {
            indexOp->accept(this);
        }
        return true;
    }
    return visitChildren(ctx);
}

std::any Compiler::visitReturnSignature(
    qasm3Parser::ReturnSignatureContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitGateModifier(qasm3Parser::GateModifierContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitScalarType(qasm3Parser::ScalarTypeContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitQubitType(qasm3Parser::QubitTypeContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitArrayType(qasm3Parser::ArrayTypeContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitArrayReferenceType(
    qasm3Parser::ArrayReferenceTypeContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitDesignator(qasm3Parser::DesignatorContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitDefcalTarget(qasm3Parser::DefcalTargetContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitDefcalArgumentDefinition(
    qasm3Parser::DefcalArgumentDefinitionContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitDefcalOperand(qasm3Parser::DefcalOperandContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitGateOperand(qasm3Parser::GateOperandContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitExternArgument(
    qasm3Parser::ExternArgumentContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitArgumentDefinition(
    qasm3Parser::ArgumentDefinitionContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitArgumentDefinitionList(
    qasm3Parser::ArgumentDefinitionListContext* ctx) {
    std::string argList{ "" };
    size_t numArrArgs = 0;
    for (size_t i = 0; i < ctx->argumentDefinition().size(); ++i) {
        FuncSignature& fnSignature = definedFuncs_.back();

        auto* arg = ctx->argumentDefinition(i);
        if (arg->scalarType()) {
            auto type = extractScalarType(ctx, arg->scalarType()->getText());
            fnSignature.argTypes.push_back(type);
            argList += type + " ";
        } else if (arg->arrayReferenceType()) {
            // depending on dim create nested vector
            auto type = extractScalarType(
                ctx, arg->arrayReferenceType()->scalarType()->getText());

            // NOTE: so expression is for #dim only
            // but can also provide just array size
            auto numDim = 1;
            if (arg->arrayReferenceType()->expression()) {
                // accept only literal number for the number of
                // dimensions
                try {
                    numDim = std::stoi(
                        arg->arrayReferenceType()->expression()->getText());
                } catch (const std::exception& err) {
                    std::cerr
                        << "Parsing "
                        << arg->arrayReferenceType()->expression()->getText()
                        << " with " << err.what()
                        << " failed at "
                           "visitArgumentDefinitionList.\n";
                    throw std::runtime_error(
                        errorMessage(ctx,
                                     "array reference arguments in functions "
                                     "accept "
                                     "only "
                                     "literal number of dimensions. e.g. #dim "
                                     "= 1."));
                }
            }
            auto vectorRefType = Generator::emitNdArray(numDim, type) + " &";
            fnSignature.argTypes.push_back(vectorRefType);
            argList += std::format("VecT{} && ", numArrArgs);
            ++numArrArgs;
        } else if (arg->qubitType() || arg->QREG()) {
            auto qubitType = "QuregSlice & ";
            fnSignature.argTypes.push_back(qubitType);
            argList += "T ";
        } else {
            throw std::runtime_error(
                errorMessage(ctx,
                             "invalid function argument type. Note CREG "
                             "argument type is "
                             "deprecated. Please use bit type instead."));
        }

        const auto id = arg->Identifier()->getText();
        argList += id;

        if (i + 1 < ctx->argumentDefinition().size()) {
            argList += ", ";
        }
    }

    return argList;
}

std::any Compiler::visitDefcalArgumentDefinitionList(
    qasm3Parser::DefcalArgumentDefinitionListContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitDefcalOperandList(
    qasm3Parser::DefcalOperandListContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitExpressionList(
    qasm3Parser::ExpressionListContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitIdentifierList(
    qasm3Parser::IdentifierListContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitGateOperandList(
    qasm3Parser::GateOperandListContext* ctx) {
    return visitChildren(ctx);
}

std::any Compiler::visitExternArgumentList(
    qasm3Parser::ExternArgumentListContext* ctx) {
    return visitChildren(ctx);
}

void Compiler::firstPass(qasm3Parser::ProgramContext* ctx) {
    QubitCountPass pass{};
    pass.visitProgram(ctx);
    totalNumQubits_ = pass.totalNumQubits();
    lastQuregName_ = pass.lastQuregName();
}

std::string Compiler::extractScalarType(antlr4::ParserRuleContext* ctx,
                                        std::string scalarType) {
    auto first = scalarType.find("[");
    auto last = scalarType.find("]", first);

    std::string typeBitWidth = "32";
    if (scalarType == "bit" || scalarType == "bool") {
        typeBitWidth = "1";
    }

    if (first != scalarType.npos && last != scalarType.npos) {
        // if calling from def statement extract bit width now
        // otherwise delay it and assign default typeBitWidth
        // also, c++ backend will expect comp time constant
        // but this will be reported to the user and is part of
        // OpenQASM specification
        if (callingFromDefStmt_) {
            typeBitWidth = scalarType.substr(first + 1, last - first - 1);
        }
        scalarType = scalarType.substr(0, first);
    }

    if (scalarType == "float") {
        return "double";
    } else if (scalarType == "angle") {
        return std::format("QasmType<unsigned int, {}, true>", typeBitWidth);
    } else if (scalarType == "uint") {
        return std::format("QasmType<unsigned int, {}>", typeBitWidth);
    } else if (scalarType == "bit") {
        return std::format("QasmType<unsigned int, {}>", typeBitWidth);
    } else if (scalarType == "int") {
        return std::format("QasmType<int, {}>", typeBitWidth);
    } else if (scalarType == "bool") {
        return std::format("QasmType<unsigned int, 1>");
    } else if (scalarType == "complex") {
        return "std::complex<double>";
    } else if (scalarType == "duration" || scalarType == "stretch") {
        return "double";
    } else {
        throw std::runtime_error(errorMessage(
            ctx, std::format("unknown scalar type: {}.", scalarType)));
    }

    return scalarType;
}

std::string Compiler::getCode() const {
    return code_.toStr();
}

std::string Compiler::getCodeTo(size_t lineNumber) const {
    return code_.toStrTo(lineNumber);
}

std::string Compiler::getCodeFrom(size_t lineNumber) const {
    return code_.toStrFrom(lineNumber);
}

void Compiler::compile(const std::string& cppComp,
                       const std::string& outFile,
                       const std::string& libPath,
                       const std::string& libs,
                       const std::string& includePath,
                       const std::string& rpathPath,
                       const bool isFormatting,
                       const bool isVerbose) {
    outputFilename_ = outFile;
    std::string outputCpp = outFile + ".cpp";
    std::string tmpCpp = outputCpp;

    if (isFormatting) {
        tmpCpp = "tmp" + tmpCpp;
    }
    std::ofstream out(tmpCpp);

    auto codeStr = code_.toStr();
    out << codeStr;

    out.close();

    if (isFormatting) {
        auto sysExitCode = std::system(
            std::string("clang-format " + tmpCpp + " > " + outputCpp).c_str());
        if (sysExitCode != 0) {
            throw std::runtime_error(Utils::externalErrorMessage(
                std::format("in compile(): clang-format returned "
                            "with error code: {}.",
                            sysExitCode)));
        }

        sysExitCode = std::system(std::string("rm " + tmpCpp).c_str());
        if (sysExitCode != 0) {
            throw std::runtime_error(Utils::externalErrorMessage(
                std::format("in compile(): rm {} returned with error code: {}.",
                            tmpCpp, sysExitCode)));
        }
    }
    std::string compilerCall = cppComp + std::string(" -std=c++20 -fopenmp ")
                               + outputCpp + std::string(" -o ") + outFile
                               + includePath + libPath + libs + rpathPath
                               + " -lQuEST ";

    if (isVerbose) {
        Utils::print(std::format("{}\n", compilerCall));
    }

    auto sysExitCode = std::system(compilerCall.c_str());    // compile
    if (sysExitCode != 0) {
        throw std::runtime_error(Utils::externalErrorMessage(std::format(
            "in compile(): call to backend ({}) returned "
            "with error code: {}.\n\n"
            "This might be caused by a known "
            "error when QuEST was built as a static library and did "
            "not pull it's dependencies correctly.\n"
            "Consider adding OpenMP library to the qasc call.",
            compilerCall, sysExitCode)));
    }
}

void Compiler::initQuantumBackend() {
    if (!isQbackendInit_ && (!callingFromDefStmt_ || callingFromMainFn_)
        && !callingFromGateDeclaration_) {
        isQbackendInit_ = true;
        if (totalNumQubits_ == "0") {
            totalNumQubits_ = "1";
        }

        if (!includeCodeForMainFn_.empty()) {
            for (size_t i = firstLineInMainIdx_;
                 i < firstLineInMainIdx_ + includeCodeForMainFn_.size(); ++i) {
                auto& quregLine = code_.at(i);
                auto pos = quregLine.find("createQureg");
                if (pos != std::string::npos) {
                    std::string newQuregLine;
                    pos = quregLine.find("(", pos);
                    const auto closingBracketPos = quregLine.find(")", pos);
                    newQuregLine = quregLine.substr(0, pos + 1);
                    newQuregLine += totalNumQubits_;
                    newQuregLine += quregLine.substr(closingBracketPos);
                    code_.replace(i, newQuregLine);
                }
            }

            return;
        }

        code_.add(Generator::emitQuestInit());
        code_.add(
            "if (!isQascInternalQuestQuregInit) "
            "{\nisQascInternalQuestQuregInit = true;\n");
        code_.add(std::format("{} = createQureg({});\n", qascInternalQuESTQureg,
                              totalNumQubits_));
        code_.add("}\n");
    }
}

void Compiler::tryRegisterMainFn() {
    if (callingFromDefStmt_ || callingFromGateDeclaration_
        || callingFromDeclaration_) {
        return;
    }

    if (callingFromInclude_) {
        if (includeMainLineNumber_ == -1) {
            includeMainLineNumber_ = code_.size() + 1;
        }
        return;
    }

    if (!startedMain_ && scopeLvl_ == 0) {
        code_.add("int main(int argc, char* argv[]) {\n");
        firstLineInMainIdx_ = code_.size();
        auto codeFromIncludesVec = getCodeFromIncludes();
        std::string codeFromIncludes;
        for (const auto& code : codeFromIncludesVec) {
            codeFromIncludes += code;
        }
        code_.add(codeFromIncludes);
        startedMain_ = true;
    }
}

void Compiler::tryFinalizingMainFn() {
    if (startedMain_ && scopeLvl_ == 0) {
        code_.add("\nreturn 0;\n}\n");    // end main()
        startedMain_ = false;
    }
}

void Compiler::handleGateModifier(
    const std::vector<qasm3Parser::GateModifierContext*>& modifiers,
    std::string& gateName,
    int& gatePower,
    std::string& gatePowerStr) {
    bool enteredPowModifier{ false };

    const std::vector<std::string> builtInCtrlGates{
        "x", "X", "y",    "z",  "h",     "s",  "t",  "u",
        "U", "p", "swap", "cz", "phase", "rx", "ry", "rz",
    };

    const std::vector<std::string> builtInInvGates{
        "x",     "X",      "y",      "z",   "h",   "s",  "sdg",  "sx",
        "t",     "tdg",    "gphase", "U",   "u1",  "u2", "u3",   "p",
        "phase", "rx",     "ry",     "rz",  "cx",  "cy", "cz",   "CX",
        "ch",    "cs",     "ct",     "cu",  "CU",  "cp", "swap", "cswap",
        "ccz",   "cphase", "crx",    "cry", "crz",
    };

    for (const auto& modifier : modifiers) {
        if (modifier->CTRL()) {
            if (modifier->expression()) {
                throw std::runtime_error(
                    errorMessage(modifier,
                                 "multi-qubit CTRL modifier is unsupported. "
                                 "Also change ctrl(1) @ -> ctrl @"));
            }

            if (std::find(builtInCtrlGates.begin(), builtInCtrlGates.end(),
                          gateName)
                == builtInCtrlGates.end()) {
                std::string errMsg
                    = "ctrl modifier can be applied only to the following "
                      "gates: ";

                for (auto& g : builtInCtrlGates) {
                    errMsg += g + " ";
                }

                throw std::runtime_error(errorMessage(modifier, errMsg));
            }

            gateName = "c" + gateName;
        }
        if (modifier->POW()) {
            if (enteredPowModifier) {
                throw std::runtime_error(
                    errorMessage(modifier,
                                 "multiple power modifiers on a single gate "
                                 "are currently "
                                 "unsupported."));
            }
            enteredPowModifier = true;
            gatePowerStr = modifier->expression()->getText();
            ++gatePower;
        }
        if (modifier->INV()) {
            if (std::find(builtInInvGates.begin(), builtInInvGates.end(),
                          gateName)
                == builtInInvGates.end()) {
                std::string errMsg
                    = "inv modifier can be applied only to the following "
                      "gates: ";

                for (auto& g : builtInInvGates) {
                    errMsg += g + " ";
                }

                throw std::runtime_error(errorMessage(modifier, errMsg));
            }

            applyingInverse_ = true;
        }
        if (modifier->NEGCTRL()) {
            if (modifier->expression()) {
                throw std::runtime_error(
                    errorMessage(modifier,
                                 "multi-qubit NEGCTRL modifier is unsupported. "
                                 "Also change negctrl(1) @ -> negctrl @"));
            }

            if (gateName[0] == 'c') {
                gateName = gateName.substr(1);
            }
            if (std::find(builtInCtrlGates.begin(), builtInCtrlGates.end(),
                          gateName)
                == builtInCtrlGates.end()) {
                std::string errMsg
                    = "negctrl modifier can be applied only to the following "
                      "gates: ";

                for (auto& g : builtInCtrlGates) {
                    errMsg += g + " ";
                }

                throw std::runtime_error(errorMessage(modifier, errMsg));
            }
        }
    }
}

std::vector<std::string> Compiler::getCodeFromIncludes() {
    std::vector<std::string> result{};
    bool foundQuantumBackendInit = false;
    for (const auto& includeCode : includeCodeForMainFn_) {
        // by default just copy the code
        std::string newIncludeCode = includeCode;
        const auto pos = includeCode.find("if (!isQuESTEnvInit)");

        if (pos != std::string::npos) {
            if (!foundQuantumBackendInit) {
                // copy in first occurence of quantum backend init
                foundQuantumBackendInit = true;
            } else {
                // if backend is already init, skip all subsequent ones

                // get everything up to opening quantum backend guard
                newIncludeCode = includeCode.substr(0, pos);

                // exploiting that we know the structure of generated
                // code, find snd }
                size_t rightCurlyBracketPos = includeCode.find("}", pos);
                const auto endQbackendInitPos
                    = includeCode.find("}", rightCurlyBracketPos + 1);

                // get everything past } closing qbackend init
                newIncludeCode += includeCode.substr(endQbackendInitPos);
            }
        }

        result.emplace_back(newIncludeCode);
    }
    return result;
}
