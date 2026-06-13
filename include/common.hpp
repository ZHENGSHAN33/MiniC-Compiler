#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace minic {

struct SourceLocation {
    int line = 1;
    int column = 1;
};

struct CompileError {
    std::string stage;
    SourceLocation loc;
    std::string message;

    std::string str() const {
        std::ostringstream out;
        out << "[" << stage << "] line " << loc.line << ", column " << loc.column << ": " << message;
        return out.str();
    }
};

enum class TokenKind {
    KW_INT, KW_BOOL, KW_IF, KW_ELSE, KW_WHILE,
    KW_RETURN, KW_BREAK, KW_CONTINUE,
    KW_READ, KW_WRITE, KW_TRUE, KW_FALSE,
    IDENT, INT_LITERAL,
    PLUS, MINUS, STAR, SLASH, PERCENT,
    ASSIGN, EQ, NE, LT, LE, GT, GE,
    AND, OR, NOT,
    SEMICOLON, COMMA, LPAREN, RPAREN, LBRACE, RBRACE,
    END_OF_FILE
};

struct Token {
    TokenKind kind;
    std::string lexeme;
    SourceLocation loc;
};

using TokenList = std::vector<Token>;

enum class TypeKind { Int, Bool, Void, Error };

enum class ASTKind {
    Program, Function, Block, VarDecl,
    IfStmt, WhileStmt, BreakStmt, ContinueStmt,
    ReturnStmt, ReadStmt, WriteStmt, AssignStmt,
    BinaryExpr, UnaryExpr, IntLiteral, BoolLiteral, Identifier
};

struct ASTNode {
    ASTKind kind;
    std::string text;
    TypeKind type = TypeKind::Error;
    SourceLocation loc;
    std::vector<std::unique_ptr<ASTNode>> children;

    ASTNode(ASTKind k, std::string t, SourceLocation p)
        : kind(k), text(std::move(t)), loc(p) {}
};

enum class SymbolKind { Variable, Function };

struct Symbol {
    std::string name;
    SymbolKind kind;
    TypeKind type;
    int scopeLevel = 0;
    SourceLocation declaredAt;
};

enum class IROp {
    ADD, SUB, MUL, DIV, MOD,
    ASSIGN,
    LT, LE, GT, GE, EQ, NE,
    AND, OR, NOT, NEG,
    LABEL, GOTO, IF_FALSE,
    READ, WRITE, RETURN
};

struct Quad {
    IROp op;
    std::string arg1;
    std::string arg2;
    std::string result;
};

using IRList = std::vector<Quad>;

struct VMInst {
    std::string op;
    std::string arg;
};

} // namespace minic
