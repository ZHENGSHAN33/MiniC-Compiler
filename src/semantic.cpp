#include "../include/semantic.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

// ============ Scope ʵ�� ============

bool Scope::declare(const std::string& name, const Symbol& sym) {
    if (symbols.find(name) != symbols.end()) {
        return false;  
    }
    symbols[name] = sym;
    return true;
}

Symbol* Scope::lookup(const std::string& name) {
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return &it->second;
    }
    return nullptr;
}

// ============ SemanticAnalyzer ʵ�� ============

SemanticAnalyzer::SemanticAnalyzer()
    : currentScopeLevel(0), inLoop(false), expectedReturnType(TypeKind::INT), hasError(false) {
}

bool SemanticAnalyzer::analyze(ASTNode* ast) {
    if (!ast) return true;

    errors.clear();
    hasError = false;
    scopeStack.clear();
    currentScopeLevel = 0;
    inLoop = false;

    // ����ȫ��������
    enterScope();

    // ��ʼ����AST
    visitProgram(ast);

    // �˳�ȫ��������
    exitScope();

    return !hasError;
}

// ========== ���ű����� ==========

void SemanticAnalyzer::enterScope() {
    currentScopeLevel++;
    scopeStack.push_back(std::make_unique<Scope>(currentScopeLevel));
}

void SemanticAnalyzer::exitScope() {
    if (!scopeStack.empty()) {
        scopeStack.pop_back();
        currentScopeLevel--;
    }
}

bool SemanticAnalyzer::declareSymbol(const std::string& name, TypeKind type,
    SymbolKind kind, const SourceLocation& loc) {
    if (scopeStack.empty()) return false;

    // ��鵱ǰ�������Ƿ��Ѵ���ͬ������
    if (lookupCurrentScope(name)) {
        addError("�ظ��������� '" + name + "'", loc);
        return false;
    }

    Symbol sym(name, type, kind, currentScopeLevel, loc);
    return scopeStack.back()->declare(name, sym);
}

Symbol* SemanticAnalyzer::lookupSymbol(const std::string& name) {
    // ���ڲ㵽������
    for (int i = scopeStack.size() - 1; i >= 0; i--) {
        Symbol* sym = scopeStack[i]->lookup(name);
        if (sym) return sym;
    }
    return nullptr;
}

Symbol* SemanticAnalyzer::lookupCurrentScope(const std::string& name) {
    if (scopeStack.empty()) return nullptr;
    return scopeStack.back()->lookup(name);
}

// ========== AST���� ==========

void SemanticAnalyzer::visitProgram(ASTNode* node) {
    if (!node || node->kind != ASTKind::PROGRAM) return;

    for (auto* child : node->children) {
        visitFunction(child);
    }
}

void SemanticAnalyzer::visitFunction(ASTNode* node) {
    if (!node || node->kind != ASTKind::FUNCTION) return;

    // ��ȡ������
    std::string funcName;
    TypeKind returnType = TypeKind::INT;

    // �ӽڵ�����ȡ��Ϣ�����躯���ڵ�洢�����ƺͷ������ͣ�
    // ������Ҫ����ʵ��AST�ṹ������
    for (auto* child : node->children) {
        if (child->kind == ASTKind::IDENTIFIER) {
            funcName = child->value;
        }
    }

    // �����������ţ���ѡ��
    // ���뺯��������
    enterScope();

    // ���������壨Block��
    for (auto* child : node->children) {
        if (child->kind == ASTKind::BLOCK) {
            visitBlock(child);
        }
    }

    exitScope();
}

void SemanticAnalyzer::visitBlock(ASTNode* node) {
    if (!node || node->kind != ASTKind::BLOCK) return;

    enterScope();  // ����鴴����������

    for (auto* child : node->children) {
        switch (child->kind) {
        case ASTKind::VAR_DECL:
            visitVarDecl(child);
            break;
        case ASTKind::ASSIGN_STMT:
            visitAssignStmt(child);
            break;
        case ASTKind::IF_STMT:
            visitIfStmt(child);
            break;
        case ASTKind::WHILE_STMT:
            visitWhileStmt(child);
            break;
        case ASTKind::BREAK_STMT:
            visitBreakStmt(child);
            break;
        case ASTKind::CONTINUE_STMT:
            visitContinueStmt(child);
            break;
        case ASTKind::RETURN_STMT:
            visitReturnStmt(child);
            break;
        case ASTKind::READ_STMT:
            visitReadStmt(child);
            break;
        case ASTKind::WRITE_STMT:
            visitWriteStmt(child);
            break;
        default:
            break;
        }
    }

    exitScope();
}

void SemanticAnalyzer::visitVarDecl(ASTNode* node) {
    if (!node || node->kind != ASTKind::VAR_DECL) return;

    // ���� varDecl �ṹ����һ�����������ͣ��ڶ����Ǳ�ʶ��
    TypeKind varType = TypeKind::INT;
    std::string varName;
    SourceLocation loc = node->loc;

    for (auto* child : node->children) {
        if (child->kind == ASTKind::IDENTIFIER) {
            varName = child->value;
            loc = child->loc;
        }
    }

    if (!varName.empty()) {
        declareSymbol(varName, varType, SymbolKind::VARIABLE, loc);
    }
}

void SemanticAnalyzer::visitAssignStmt(ASTNode* node) {
    if (!node || node->kind != ASTKind::ASSIGN_STMT) return;

    // ���� assign �ṹ����ֵ����ʶ��������ֵ������ʽ��
    ASTNode* left = nullptr;
    ASTNode* right = nullptr;

    if (node->children.size() >= 2) {
        left = node->children[0];
        right = node->children[1];
    }

    if (left && left->kind == ASTKind::IDENTIFIER) {
        // �������Ƿ�����
        Symbol* sym = lookupSymbol(left->value);
        if (!sym) {
            addError("δ�����ı��� '" + left->value + "'", left->loc);
            return;
        }

        // �����ֵ�����Ƿ�ƥ��
        if (right) {
            TypeKind rightType = visitExpr(right);
            if (!isTypeCompatible(sym->type, rightType)) {
                addError("���Ͳ�ƥ�䣺���ܽ� '" + typeToString(rightType) +
                    "' ��ֵ�� '" + typeToString(sym->type) + "'", right->loc);
            }
        }
    }
}

void SemanticAnalyzer::visitIfStmt(ASTNode* node) {
    if (!node || node->kind != ASTKind::IF_STMT) return;

    // ���� if �ṹ����������ʽ��then��֧��else��֧����ѡ��
    if (node->children.size() >= 2) {
        ASTNode* cond = node->children[0];
        ASTNode* thenBranch = node->children[1];
        ASTNode* elseBranch = (node->children.size() > 2) ? node->children[2] : nullptr;

        // ������������� bool ����
        if (cond) {
            TypeKind condType = visitExpr(cond);
            if (condType != TypeKind::BOOL) {
                addError("if ���������� bool ���ͣ�ʵ��Ϊ '" + typeToString(condType) + "'", cond->loc);
            }
        }

        // ��� then ��֧
        if (thenBranch) visitBlock(thenBranch);

        // ��� else ��֧
        if (elseBranch) visitBlock(elseBranch);
    }
}

void SemanticAnalyzer::visitWhileStmt(ASTNode* node) {
    if (!node || node->kind != ASTKind::WHILE_STMT) return;

    // ���� while �ṹ����������ʽ��ѭ����
    if (node->children.size() >= 2) {
        ASTNode* cond = node->children[0];
        ASTNode* body = node->children[1];

        // ������������� bool ����
        if (cond) {
            TypeKind condType = visitExpr(cond);
            if (condType != TypeKind::BOOL) {
                addError("while ���������� bool ���ͣ�ʵ��Ϊ '" + typeToString(condType) + "'", cond->loc);
            }
        }

        // ��ǽ���ѭ��
        bool oldInLoop = inLoop;
        inLoop = true;

        // ���ѭ����
        if (body) visitBlock(body);

        inLoop = oldInLoop;
    }
}

void SemanticAnalyzer::visitBreakStmt(ASTNode* node) {
    if (!node) return;

    if (!inLoop) {
        addError("break ��������ѭ����ʹ��", node->loc);
    }
}

void SemanticAnalyzer::visitContinueStmt(ASTNode* node) {
    if (!node) return;

    if (!inLoop) {
        addError("continue ��������ѭ����ʹ��", node->loc);
    }
}

void SemanticAnalyzer::visitReturnStmt(ASTNode* node) {
    if (!node || node->kind != ASTKind::RETURN_STMT) return;

    // ����з���ֵ����ʽ
    if (!node->children.empty()) {
        ASTNode* expr = node->children[0];
        if (expr) {
            TypeKind retType = visitExpr(expr);
            if (!isTypeCompatible(expectedReturnType, retType)) {
                addError("�������Ͳ�ƥ�䣺���� '" + typeToString(expectedReturnType) +
                    "'��ʵ��Ϊ '" + typeToString(retType) + "'", expr->loc);
            }
        }
    }
}

void SemanticAnalyzer::visitReadStmt(ASTNode* node) {
    if (!node || node->kind != ASTKind::READ_STMT) return;

    // read �Ĳ���Ӧ����һ��������ʶ��
    if (!node->children.empty()) {
        ASTNode* var = node->children[0];
        if (var && var->kind == ASTKind::IDENTIFIER) {
            if (!lookupSymbol(var->value)) {
                addError("δ�����ı��� '" + var->value + "'", var->loc);
            }
        }
    }
}

void SemanticAnalyzer::visitWriteStmt(ASTNode* node) {
    if (!node || node->kind != ASTKind::WRITE_STMT) return;

    // write �Ĳ�����һ������ʽ
    if (!node->children.empty()) {
        ASTNode* expr = node->children[0];
        if (expr) {
            visitExpr(expr);  // ������ʽ�еı����Ƿ�����
        }
    }
}

// ========== ����ʽ���ͼ�� ==========

TypeKind SemanticAnalyzer::visitExpr(ASTNode* node) {
    if (!node) return TypeKind::UNKNOWN;

    switch (node->kind) {
    case ASTKind::BINARY_EXPR:
        return visitBinaryExpr(node);
    case ASTKind::UNARY_EXPR:
        return visitUnaryExpr(node);
    case ASTKind::IDENTIFIER:
        return visitIdentifier(node);
    case ASTKind::INT_LITERAL:
        return visitIntLiteral(node);
    case ASTKind::BOOL_LITERAL:
        return visitBoolLiteral(node);
    default:
        return TypeKind::UNKNOWN;
    }
}

TypeKind SemanticAnalyzer::visitBinaryExpr(ASTNode* node) {
    if (!node || node->children.size() < 2) return TypeKind::UNKNOWN;

    ASTNode* left = node->children[0];
    ASTNode* right = node->children[1];

    TypeKind leftType = visitExpr(left);
    TypeKind rightType = visitExpr(right);

    // ���ݲ������жϽ������
    std::string op = node->value;

    // �����������Ҫ�����Ҷ��� int������� int
    if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
        if (leftType != TypeKind::INT) {
            addError("������������������������ int ����", left->loc);
        }
        if (rightType != TypeKind::INT) {
            addError("������������Ҳ����������� int ����", right->loc);
        }
        return TypeKind::INT;
    }

    // ��ϵ�������Ҫ�����Ҷ��� int������� bool
    if (op == "<" || op == "<=" || op == ">" || op == ">=") {
        if (leftType != TypeKind::INT) {
            addError("��ϵ�������������������� int ����", left->loc);
        }
        if (rightType != TypeKind::INT) {
            addError("��ϵ��������Ҳ����������� int ����", right->loc);
        }
        return TypeKind::BOOL;
    }

    // ����������Ҫ������������ͬ������� bool
    if (op == "==" || op == "!=") {
        if (leftType != rightType) {
            addError("�����������������ͱ�����ͬ", node->loc);
        }
        return TypeKind::BOOL;
    }

    // �߼��������Ҫ�����Ҷ��� bool������� bool
    if (op == "&&" || op == "||") {
        if (leftType != TypeKind::BOOL) {
            addError("�߼��������������������� bool ����", left->loc);
        }
        if (rightType != TypeKind::BOOL) {
            addError("�߼���������Ҳ����������� bool ����", right->loc);
        }
        return TypeKind::BOOL;
    }

    return TypeKind::UNKNOWN;
}

TypeKind SemanticAnalyzer::visitUnaryExpr(ASTNode* node) {
    if (!node || node->children.empty()) return TypeKind::UNKNOWN;

    ASTNode* operand = node->children[0];
    std::string op = node->value;
    TypeKind operandType = visitExpr(operand);

    if (op == "-") {
        if (operandType != TypeKind::INT) {
            addError("ȡ��������Ĳ����������� int ����", operand->loc);
        }
        return TypeKind::INT;
    }

    if (op == "!") {
        if (operandType != TypeKind::BOOL) {
            addError("�߼���������Ĳ����������� bool ����", operand->loc);
        }
        return TypeKind::BOOL;
    }

    return TypeKind::UNKNOWN;
}

TypeKind SemanticAnalyzer::visitIdentifier(ASTNode* node) {
    if (!node) return TypeKind::UNKNOWN;

    Symbol* sym = lookupSymbol(node->value);
    if (!sym) {
        addError("δ�����ı��� '" + node->value + "'", node->loc);
        return TypeKind::UNKNOWN;
    }

    return sym->type;
}

TypeKind SemanticAnalyzer::visitIntLiteral(ASTNode* node) {
    return TypeKind::INT;
}

TypeKind SemanticAnalyzer::visitBoolLiteral(ASTNode* node) {
    return TypeKind::BOOL;
}

// ========== �������� ==========

void SemanticAnalyzer::addError(const std::string& msg, const SourceLocation& loc) {
    std::ostringstream oss;
    oss << "[SemanticError] line " << loc.line << ", column " << loc.column
        << ": " << msg;
    errors.push_back(oss.str());
    hasError = true;
}

bool SemanticAnalyzer::isTypeCompatible(TypeKind left, TypeKind right) {
    // ��ȫƥ��� UNKNOWN ���ͣ�δ�ƶϳ���
    if (left == right) return true;
    if (left == TypeKind::UNKNOWN || right == TypeKind::UNKNOWN) return true;
    return false;
}

std::string SemanticAnalyzer::typeToString(TypeKind type) const {
    switch (type) {
    case TypeKind::INT: return "int";
    case TypeKind::BOOL: return "bool";
    case TypeKind::VOID: return "void";
    default: return "unknown";
    }
}

void SemanticAnalyzer::printSymbolTable() const {
    std::cout << getSymbolTableString() << std::endl;
}

std::string SemanticAnalyzer::getSymbolTableString() const {
    std::ostringstream oss;
    oss << "\n========== Symbol Table ==========\n";
    oss << std::left << std::setw(20) << "Name"
        << std::setw(15) << "Type"
        << std::setw(15) << "Kind"
        << std::setw(10) << "Scope"
        << std::setw(10) << "Init"
        << "Location\n";
    oss << "--------------------------------------------------------------\n";

    // ��������������
    for (const auto& scope : scopeStack) {
        // ������Ҫͨ��ĳ�ַ�ʽ�����������еķ���
        // ���� Scope ��û���ṩ�����ӿڣ��������Ϊʾ��
        // ʵ��ʵ��ʱ������ Scope �������� getSymbols() ����
    }

    oss << "==============================================================\n";
    return oss.str();
}

// ========== �ⲿ�ӿں��� ==========

bool semanticCheck(ASTNode* ast, std::vector<std::string>& errors) {
    SemanticAnalyzer analyzer;
    bool result = analyzer.analyze(ast);
    errors = analyzer.getErrors();
    return result;
}

void printSymbolTable(ASTNode* ast) {
    SemanticAnalyzer analyzer;
    analyzer.analyze(ast);
    analyzer.printSymbolTable();
}