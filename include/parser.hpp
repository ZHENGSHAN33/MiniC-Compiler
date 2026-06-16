#pragma once

#include "common.hpp"

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace minic {

std::string astName(ASTKind kind);

class Parser {
public:
    explicit Parser(const TokenList& tokens);

    std::unique_ptr<ASTNode> parseProgram();

    const std::vector<CompileError>& errors() const;

private:
    const TokenList& tokens_;
    size_t pos_ = 0;
    std::vector<CompileError> errors_;

    const Token& current() const;
    const Token& previous() const;

    bool check(TokenKind kind) const;
    bool match(TokenKind kind);

    Token consume(TokenKind kind, const std::string& message);
    void error(SourceLocation loc, const std::string& message);

    void synchronizeStmt();
    void synchronizeTop();

    TypeKind parseType();

    std::unique_ptr<ASTNode> parseFunction();
    std::unique_ptr<ASTNode> parseBlock();
    std::unique_ptr<ASTNode> parseStmt();

    std::unique_ptr<ASTNode> parseVarDecl();
    std::unique_ptr<ASTNode> parseIf(SourceLocation loc);
    std::unique_ptr<ASTNode> parseWhile(SourceLocation loc);
    std::unique_ptr<ASTNode> parseReturn(SourceLocation loc);
    std::unique_ptr<ASTNode> parseRead(SourceLocation loc);
    std::unique_ptr<ASTNode> parseWrite(SourceLocation loc);
    std::unique_ptr<ASTNode> parseAssign();

    std::unique_ptr<ASTNode> parseExpr();
    std::unique_ptr<ASTNode> parseOr();
    std::unique_ptr<ASTNode> parseAnd();
    std::unique_ptr<ASTNode> parseEquality();
    std::unique_ptr<ASTNode> parseRelation();
    std::unique_ptr<ASTNode> parseAdd();
    std::unique_ptr<ASTNode> parseMul();
    std::unique_ptr<ASTNode> parseUnary();
    std::unique_ptr<ASTNode> parsePrimary();

    std::unique_ptr<ASTNode> binary(
        const std::string& op,
        std::unique_ptr<ASTNode> left,
        std::unique_ptr<ASTNode> right,
        SourceLocation loc
    );
};

void printAST(std::ostream& out, const ASTNode* node, int depth = 0);

class DotPrinter {
public:
    std::string print(const ASTNode* root);

private:
    std::ostringstream out_;
    int nextId_ = 0;

    int visit(const ASTNode* node);
};

} // namespace minic