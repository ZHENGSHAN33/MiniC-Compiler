#ifndef SEMANTIC_HPP
#define SEMANTIC_HPP

#include "common.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

// ============ 符号表相关 ============

// 符号表条目
struct Symbol {
    std::string name;
    TypeKind type;
    SymbolKind kind;     // 变量、函数等
    int scopeLevel;      // 作用域层级
    bool isInitialized;  // 是否已初始化
    SourceLocation loc;  // 声明位置

    Symbol() : type(TypeKind::INT), kind(SymbolKind::VARIABLE),
        scopeLevel(0), isInitialized(false) {
    }
    Symbol(const std::string& n, TypeKind t, SymbolKind k, int level, const SourceLocation& l)
        : name(n), type(t), kind(k), scopeLevel(level), isInitialized(false), loc(l) {
    }
};

// 作用域（用哈希表存储当前作用域的符号）
class Scope {
public:
    Scope(int level) : level(level) {}

    // 在当前作用域声明符号
    bool declare(const std::string& name, const Symbol& sym);

    // 在当前作用域查找符号
    Symbol* lookup(const std::string& name);

    int getLevel() const { return level; }

private:
    int level;
    std::unordered_map<std::string, Symbol> symbols;
};

// ============ 语义分析器 ============

class SemanticAnalyzer {
public:
    SemanticAnalyzer();

    // 主入口：对AST进行语义分析
    bool analyze(ASTNode* ast);

    // 获取所有语义错误
    const std::vector<std::string>& getErrors() const { return errors; }

    // 输出符号表（用于调试/答辩）
    void printSymbolTable() const;

    // 获取符号表的字符串表示
    std::string getSymbolTableString() const;

private:
    // ====== 符号表管理 ======
    void enterScope();
    void exitScope();
    bool declareSymbol(const std::string& name, TypeKind type, SymbolKind kind, const SourceLocation& loc);
    Symbol* lookupSymbol(const std::string& name);
    Symbol* lookupCurrentScope(const std::string& name);

    // ====== AST遍历 ======
    void visitProgram(ASTNode* node);
    void visitFunction(ASTNode* node);
    void visitBlock(ASTNode* node);
    void visitVarDecl(ASTNode* node);
    void visitAssignStmt(ASTNode* node);
    void visitIfStmt(ASTNode* node);
    void visitWhileStmt(ASTNode* node);
    void visitBreakStmt(ASTNode* node);
    void visitContinueStmt(ASTNode* node);
    void visitReturnStmt(ASTNode* node);
    void visitReadStmt(ASTNode* node);
    void visitWriteStmt(ASTNode* node);

    // ====== 表达式类型检查 ======
    TypeKind visitExpr(ASTNode* node);
    TypeKind visitBinaryExpr(ASTNode* node);
    TypeKind visitUnaryExpr(ASTNode* node);
    TypeKind visitIdentifier(ASTNode* node);
    TypeKind visitIntLiteral(ASTNode* node);
    TypeKind visitBoolLiteral(ASTNode* node);

    // ====== 辅助函数 ======
    void addError(const std::string& msg, const SourceLocation& loc);
    bool isTypeCompatible(TypeKind left, TypeKind right);
    std::string typeToString(TypeKind type) const;

    // ====== 成员变量 ======
    std::vector<std::unique_ptr<Scope>> scopeStack;  // 作用域栈
    std::vector<std::string> errors;                  // 错误列表
    int currentScopeLevel;                            // 当前作用域层级
    bool inLoop;                                     // 是否在循环内
    TypeKind expectedReturnType;                     // 函数期望的返回类型
    bool hasError;                                   // 是否发生错误
};

// ============ 辅助函数（供外部调用） ============

// 对AST进行语义分析，返回是否成功
bool semanticCheck(ASTNode* ast, std::vector<std::string>& errors);

// 打印符号表
void printSymbolTable(ASTNode* ast);

#endif // SEMANTIC_HPP