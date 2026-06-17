#pragma once

#include "common.hpp"

#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>

namespace minic {

class Semantic {
public:
    void analyze(ASTNode* root);

    const std::vector<CompileError>& errors() const;
    void printSymbols(std::ostream& out) const;

private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes_;
    std::vector<Symbol> allSymbols_;
    std::vector<CompileError> errors_;
    TypeKind currentReturn_ = TypeKind::Void;
    int loopDepth_ = 0;

    void enterScope();
    void exitScope();
    int scopeLevel() const;

    void error(SourceLocation loc, const std::string& message);
    bool declare(const Symbol& symbol);
    Symbol* lookup(const std::string& name);

    void analyzeFunction(ASTNode* fn);
    void analyzeBlock(ASTNode* block, bool nested = true);
    void analyzeStmt(ASTNode* node);
    TypeKind analyzeExpr(ASTNode* node);
};

std::string typeName(TypeKind type);

} // namespace minic
