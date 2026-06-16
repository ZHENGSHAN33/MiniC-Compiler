#include "parser.hpp"

#include <algorithm>
#include <ostream>
#include <utility>

namespace minic {

namespace {

std::string typeNameForAST(TypeKind type) {
    switch (type) {
        case TypeKind::Int: return "int";
        case TypeKind::Bool: return "bool";
        case TypeKind::Void: return "void";
        case TypeKind::Error: return "error";
    }
    return "error";
}

std::unique_ptr<ASTNode> makeNode(ASTKind kind, const std::string& text, SourceLocation loc) {
    return std::make_unique<ASTNode>(kind, text, loc);
}

} // namespace

std::string astName(ASTKind kind) {
    switch (kind) {
        case ASTKind::Program: return "Program";
        case ASTKind::Function: return "Function";
        case ASTKind::Block: return "Block";
        case ASTKind::VarDecl: return "VarDecl";
        case ASTKind::IfStmt: return "IfStmt";
        case ASTKind::WhileStmt: return "WhileStmt";
        case ASTKind::BreakStmt: return "BreakStmt";
        case ASTKind::ContinueStmt: return "ContinueStmt";
        case ASTKind::ReturnStmt: return "ReturnStmt";
        case ASTKind::ReadStmt: return "ReadStmt";
        case ASTKind::WriteStmt: return "WriteStmt";
        case ASTKind::AssignStmt: return "AssignStmt";
        case ASTKind::BinaryExpr: return "BinaryExpr";
        case ASTKind::UnaryExpr: return "UnaryExpr";
        case ASTKind::IntLiteral: return "IntLiteral";
        case ASTKind::BoolLiteral: return "BoolLiteral";
        case ASTKind::Identifier: return "Identifier";
    }
    return "Node";
}

Parser::Parser(const TokenList& tokens) : tokens_(tokens) {}

std::unique_ptr<ASTNode> Parser::parseProgram() {
    auto program = makeNode(ASTKind::Program, "", current().loc);
    while (!check(TokenKind::END_OF_FILE)) {
        program->children.push_back(parseFunction());
        if (!errors_.empty()) synchronizeTop();
    }
    return program;
}

const std::vector<CompileError>& Parser::errors() const {
    return errors_;
}

const Token& Parser::current() const {
    return tokens_[std::min(pos_, tokens_.size() - 1)];
}

const Token& Parser::previous() const {
    return tokens_[pos_ - 1];
}

bool Parser::check(TokenKind kind) const {
    return current().kind == kind;
}

bool Parser::match(TokenKind kind) {
    if (!check(kind)) return false;
    ++pos_;
    return true;
}

Token Parser::consume(TokenKind kind, const std::string& message) {
    if (check(kind)) return tokens_[pos_++];
    error(current().loc, message);
    return {kind, "", current().loc};
}

void Parser::error(SourceLocation loc, const std::string& message) {
    errors_.push_back({"SyntaxError", loc, message});
}

void Parser::synchronizeStmt() {
    while (!check(TokenKind::END_OF_FILE) && !check(TokenKind::RBRACE)) {
        if (match(TokenKind::SEMICOLON)) return;
        if (check(TokenKind::KW_INT) || check(TokenKind::KW_BOOL) || check(TokenKind::KW_IF) ||
            check(TokenKind::KW_WHILE) || check(TokenKind::KW_RETURN)) {
            return;
        }
        ++pos_;
    }
}

void Parser::synchronizeTop() {
    while (!check(TokenKind::END_OF_FILE) && !check(TokenKind::KW_INT)) {
        ++pos_;
    }
}

TypeKind Parser::parseType() {
    if (match(TokenKind::KW_INT)) return TypeKind::Int;
    if (match(TokenKind::KW_BOOL)) return TypeKind::Bool;
    error(current().loc, "expected type name");
    return TypeKind::Error;
}

std::unique_ptr<ASTNode> Parser::parseFunction() {
    SourceLocation loc = current().loc;
    TypeKind ret = parseType();

    Token name = consume(TokenKind::IDENT, "expected function name");

    consume(TokenKind::LPAREN, "expected '(' after function name");
    consume(TokenKind::RPAREN, "expected ')' after function parameters");

    auto fn = makeNode(ASTKind::Function, name.lexeme, loc);
    fn->type = ret;
    fn->children.push_back(parseBlock());

    return fn;
}

std::unique_ptr<ASTNode> Parser::parseBlock() {
    SourceLocation loc = current().loc;

    consume(TokenKind::LBRACE, "expected '{' before block");

    auto block = makeNode(ASTKind::Block, "", loc);

    while (!check(TokenKind::RBRACE) && !check(TokenKind::END_OF_FILE)) {
        block->children.push_back(parseStmt());
    }

    consume(TokenKind::RBRACE, "expected '}' after block");

    return block;
}

std::unique_ptr<ASTNode> Parser::parseStmt() {
    try {
        if (check(TokenKind::KW_INT) || check(TokenKind::KW_BOOL)) {
            return parseVarDecl();
        }

        if (check(TokenKind::LBRACE)) {
            return parseBlock();
        }

        if (match(TokenKind::KW_IF)) {
            return parseIf(previous().loc);
        }

        if (match(TokenKind::KW_WHILE)) {
            return parseWhile(previous().loc);
        }

        if (match(TokenKind::KW_BREAK)) {
            auto node = makeNode(ASTKind::BreakStmt, "", previous().loc);
            consume(TokenKind::SEMICOLON, "expected ';' after break");
            return node;
        }

        if (match(TokenKind::KW_CONTINUE)) {
            auto node = makeNode(ASTKind::ContinueStmt, "", previous().loc);
            consume(TokenKind::SEMICOLON, "expected ';' after continue");
            return node;
        }

        if (match(TokenKind::KW_RETURN)) {
            return parseReturn(previous().loc);
        }

        if (match(TokenKind::KW_READ)) {
            return parseRead(previous().loc);
        }

        if (match(TokenKind::KW_WRITE)) {
            return parseWrite(previous().loc);
        }

        if (check(TokenKind::IDENT)) {
            return parseAssign();
        }

        error(current().loc, "expected statement");
        ++pos_;
        return makeNode(ASTKind::Block, "", current().loc);
    } catch (...) {
        synchronizeStmt();
        return makeNode(ASTKind::Block, "", current().loc);
    }
}

std::unique_ptr<ASTNode> Parser::parseVarDecl() {
    SourceLocation loc = current().loc;

    TypeKind type = parseType();
    Token name = consume(TokenKind::IDENT, "expected variable name");

    consume(TokenKind::SEMICOLON, "expected ';' after variable declaration");

    auto node = makeNode(ASTKind::VarDecl, name.lexeme, loc);
    node->type = type;

    return node;
}

std::unique_ptr<ASTNode> Parser::parseIf(SourceLocation loc) {
    auto node = makeNode(ASTKind::IfStmt, "", loc);

    consume(TokenKind::LPAREN, "expected '(' after if");
    node->children.push_back(parseExpr());
    consume(TokenKind::RPAREN, "expected ')' after if condition");

    node->children.push_back(parseStmt());

    if (match(TokenKind::KW_ELSE)) {
        node->children.push_back(parseStmt());
    }

    return node;
}

std::unique_ptr<ASTNode> Parser::parseWhile(SourceLocation loc) {
    auto node = makeNode(ASTKind::WhileStmt, "", loc);

    consume(TokenKind::LPAREN, "expected '(' after while");
    node->children.push_back(parseExpr());
    consume(TokenKind::RPAREN, "expected ')' after while condition");

    node->children.push_back(parseStmt());

    return node;
}

std::unique_ptr<ASTNode> Parser::parseReturn(SourceLocation loc) {
    auto node = makeNode(ASTKind::ReturnStmt, "", loc);

    node->children.push_back(parseExpr());

    consume(TokenKind::SEMICOLON, "expected ';' after return value");

    return node;
}

std::unique_ptr<ASTNode> Parser::parseRead(SourceLocation loc) {
    auto node = makeNode(ASTKind::ReadStmt, "", loc);

    consume(TokenKind::LPAREN, "expected '(' after read");

    Token name = consume(TokenKind::IDENT, "read expects a variable name");
    node->children.push_back(makeNode(ASTKind::Identifier, name.lexeme, name.loc));

    consume(TokenKind::RPAREN, "expected ')' after read argument");
    consume(TokenKind::SEMICOLON, "expected ';' after read statement");

    return node;
}

std::unique_ptr<ASTNode> Parser::parseWrite(SourceLocation loc) {
    auto node = makeNode(ASTKind::WriteStmt, "", loc);

    consume(TokenKind::LPAREN, "expected '(' after write");

    node->children.push_back(parseExpr());

    consume(TokenKind::RPAREN, "expected ')' after write argument");
    consume(TokenKind::SEMICOLON, "expected ';' after write statement");

    return node;
}

std::unique_ptr<ASTNode> Parser::parseAssign() {
    Token name = consume(TokenKind::IDENT, "expected variable name");

    auto node = makeNode(ASTKind::AssignStmt, name.lexeme, name.loc);

    consume(TokenKind::ASSIGN, "expected '=' in assignment");

    node->children.push_back(parseExpr());

    consume(TokenKind::SEMICOLON, "expected ';' after assignment");

    return node;
}

std::unique_ptr<ASTNode> Parser::parseExpr() {
    return parseOr();
}

std::unique_ptr<ASTNode> Parser::parseOr() {
    auto left = parseAnd();

    while (match(TokenKind::OR)) {
        left = binary("||", std::move(left), parseAnd(), previous().loc);
    }

    return left;
}

std::unique_ptr<ASTNode> Parser::parseAnd() {
    auto left = parseEquality();

    while (match(TokenKind::AND)) {
        left = binary("&&", std::move(left), parseEquality(), previous().loc);
    }

    return left;
}

std::unique_ptr<ASTNode> Parser::parseEquality() {
    auto left = parseRelation();

    while (match(TokenKind::EQ) || match(TokenKind::NE)) {
        std::string op = previous().lexeme;
        left = binary(op, std::move(left), parseRelation(), previous().loc);
    }

    return left;
}

std::unique_ptr<ASTNode> Parser::parseRelation() {
    auto left = parseAdd();

    while (match(TokenKind::LT) || match(TokenKind::LE) || match(TokenKind::GT) || match(TokenKind::GE)) {
        std::string op = previous().lexeme;
        left = binary(op, std::move(left), parseAdd(), previous().loc);
    }

    return left;
}

std::unique_ptr<ASTNode> Parser::parseAdd() {
    auto left = parseMul();

    while (match(TokenKind::PLUS) || match(TokenKind::MINUS)) {
        std::string op = previous().lexeme;
        left = binary(op, std::move(left), parseMul(), previous().loc);
    }

    return left;
}

std::unique_ptr<ASTNode> Parser::parseMul() {
    auto left = parseUnary();

    while (match(TokenKind::STAR) || match(TokenKind::SLASH) || match(TokenKind::PERCENT)) {
        std::string op = previous().lexeme;
        left = binary(op, std::move(left), parseUnary(), previous().loc);
    }

    return left;
}

std::unique_ptr<ASTNode> Parser::parseUnary() {
    if (match(TokenKind::NOT) || match(TokenKind::MINUS)) {
        std::string op = previous().lexeme;

        auto node = makeNode(ASTKind::UnaryExpr, op, previous().loc);
        node->children.push_back(parseUnary());

        return node;
    }

    return parsePrimary();
}

std::unique_ptr<ASTNode> Parser::parsePrimary() {
    if (match(TokenKind::INT_LITERAL)) {
        auto node = makeNode(ASTKind::IntLiteral, previous().lexeme, previous().loc);
        node->type = TypeKind::Int;
        return node;
    }

    if (match(TokenKind::KW_TRUE) || match(TokenKind::KW_FALSE)) {
        auto node = makeNode(ASTKind::BoolLiteral, previous().lexeme, previous().loc);
        node->type = TypeKind::Bool;
        return node;
    }

    if (match(TokenKind::IDENT)) {
        return makeNode(ASTKind::Identifier, previous().lexeme, previous().loc);
    }

    if (match(TokenKind::LPAREN)) {
        auto expr = parseExpr();
        consume(TokenKind::RPAREN, "expected ')' after expression");
        return expr;
    }

    error(current().loc, "expected expression");

    auto node = makeNode(ASTKind::IntLiteral, "0", current().loc);
    node->type = TypeKind::Error;

    return node;
}

std::unique_ptr<ASTNode> Parser::binary(
    const std::string& op,
    std::unique_ptr<ASTNode> left,
    std::unique_ptr<ASTNode> right,
    SourceLocation loc
) {
    auto node = makeNode(ASTKind::BinaryExpr, op, loc);

    node->children.push_back(std::move(left));
    node->children.push_back(std::move(right));

    return node;
}

void printAST(std::ostream& out, const ASTNode* node, int depth) {
    out << std::string(depth * 2, ' ') << astName(node->kind);

    if (!node->text.empty()) {
        out << " " << node->text;
    }

    if (node->type != TypeKind::Error) {
        out << " : " << typeNameForAST(node->type);
    }

    out << "\n";

    for (const auto& child : node->children) {
        printAST(out, child.get(), depth + 1);
    }
}

std::string DotPrinter::print(const ASTNode* root) {
    out_ << "digraph AST {\n  node [shape=box, fontname=\"Consolas\"];\n";

    visit(root);

    out_ << "}\n";

    return out_.str();
}

int DotPrinter::visit(const ASTNode* node) {
    int id = nextId_++;

    std::string label = astName(node->kind);

    if (!node->text.empty()) {
        label += "\\n" + node->text;
    }

    out_ << "  n" << id << " [label=\"" << label << "\"];\n";

    for (const auto& child : node->children) {
        int cid = visit(child.get());
        out_ << "  n" << id << " -> n" << cid << ";\n";
    }

    return id;
}

} // namespace minic