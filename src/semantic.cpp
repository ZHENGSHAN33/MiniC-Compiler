#include "semantic.hpp"

#include <iomanip>
#include <ostream>

namespace minic {

std::string typeName(TypeKind type) {
    switch (type) {
        case TypeKind::Int: return "int";
        case TypeKind::Bool: return "bool";
        case TypeKind::Void: return "void";
        case TypeKind::Error: return "error";
    }
    return "error";
}

void Semantic::analyze(ASTNode* root) {
    scopes_.clear();
    allSymbols_.clear();
    errors_.clear();
    loopDepth_ = 0;
    currentReturn_ = TypeKind::Void;

    if (!root) return;

    enterScope();
    for (auto& child : root->children) {
        analyzeFunction(child.get());
    }
    exitScope();
}

const std::vector<CompileError>& Semantic::errors() const {
    return errors_;
}

void Semantic::printSymbols(std::ostream& out) const {
    out << "Scope  Kind      Type   Name\n";
    for (const auto& s : allSymbols_) {
        out << std::setw(5) << s.scopeLevel << "  "
            << std::setw(8) << (s.kind == SymbolKind::Function ? "function" : "variable") << "  "
            << std::setw(5) << typeName(s.type) << "  " << s.name << "\n";
    }
}

void Semantic::enterScope() {
    scopes_.push_back({});
}

void Semantic::exitScope() {
    if (!scopes_.empty()) scopes_.pop_back();
}

int Semantic::scopeLevel() const {
    return static_cast<int>(scopes_.size()) - 1;
}

void Semantic::error(SourceLocation loc, const std::string& message) {
    errors_.push_back({"SemanticError", loc, message});
}

bool Semantic::declare(const Symbol& symbol) {
    auto& scope = scopes_.back();
    if (scope.count(symbol.name)) return false;
    scope[symbol.name] = symbol;
    allSymbols_.push_back(symbol);
    return true;
}

Symbol* Semantic::lookup(const std::string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    return nullptr;
}

void Semantic::analyzeFunction(ASTNode* fn) {
    if (!fn || fn->kind != ASTKind::Function) return;

    Symbol symbol{fn->text, SymbolKind::Function, fn->type, scopeLevel(), fn->loc};
    if (!declare(symbol)) {
        error(fn->loc, "function '" + fn->text + "' is already declared");
    }

    TypeKind previousReturn = currentReturn_;
    currentReturn_ = fn->type;

    if (!fn->children.empty()) {
        analyzeBlock(fn->children[0].get(), false);
    }

    currentReturn_ = previousReturn;
}

void Semantic::analyzeBlock(ASTNode* block, bool nested) {
    if (!block || block->kind != ASTKind::Block) return;

    if (nested) enterScope();
    for (auto& child : block->children) {
        analyzeStmt(child.get());
    }
    if (nested) exitScope();
}

void Semantic::analyzeStmt(ASTNode* node) {
    if (!node) return;

    switch (node->kind) {
        case ASTKind::Block:
            analyzeBlock(node);
            break;
        case ASTKind::VarDecl: {
            Symbol symbol{node->text, SymbolKind::Variable, node->type, scopeLevel(), node->loc};
            if (!declare(symbol)) {
                error(node->loc, "variable '" + node->text + "' is already declared in this scope");
            }
            break;
        }
        case ASTKind::AssignStmt: {
            Symbol* s = lookup(node->text);
            TypeKind rhs = node->children.empty() ? TypeKind::Error : analyzeExpr(node->children[0].get());
            if (!s) {
                error(node->loc, "variable '" + node->text + "' is not declared");
            } else if (rhs != TypeKind::Error && s->type != rhs) {
                error(node->loc, "cannot assign " + typeName(rhs) + " to " + typeName(s->type) + " variable '" + node->text + "'");
            }
            break;
        }
        case ASTKind::ReadStmt: {
            if (node->children.empty()) break;
            ASTNode* id = node->children[0].get();
            Symbol* s = lookup(id->text);
            if (!s) {
                error(id->loc, "variable '" + id->text + "' is not declared");
            } else if (s->type != TypeKind::Int) {
                error(id->loc, "read currently supports int variables only");
            }
            break;
        }
        case ASTKind::WriteStmt:
            if (!node->children.empty()) analyzeExpr(node->children[0].get());
            break;
        case ASTKind::IfStmt: {
            if (node->children.size() < 2) break;
            TypeKind cond = analyzeExpr(node->children[0].get());
            if (cond != TypeKind::Bool && cond != TypeKind::Error) {
                error(node->children[0]->loc, "if condition must be bool");
            }
            analyzeStmt(node->children[1].get());
            if (node->children.size() > 2) analyzeStmt(node->children[2].get());
            break;
        }
        case ASTKind::WhileStmt: {
            if (node->children.size() < 2) break;
            TypeKind cond = analyzeExpr(node->children[0].get());
            if (cond != TypeKind::Bool && cond != TypeKind::Error) {
                error(node->children[0]->loc, "while condition must be bool");
            }
            ++loopDepth_;
            analyzeStmt(node->children[1].get());
            --loopDepth_;
            break;
        }
        case ASTKind::BreakStmt:
            if (loopDepth_ == 0) error(node->loc, "break must be inside a loop");
            break;
        case ASTKind::ContinueStmt:
            if (loopDepth_ == 0) error(node->loc, "continue must be inside a loop");
            break;
        case ASTKind::ReturnStmt: {
            TypeKind actual = node->children.empty() ? TypeKind::Void : analyzeExpr(node->children[0].get());
            if (actual != TypeKind::Error && actual != currentReturn_) {
                error(node->loc, "return type should be " + typeName(currentReturn_));
            }
            break;
        }
        default:
            break;
    }
}

TypeKind Semantic::analyzeExpr(ASTNode* node) {
    if (!node) return TypeKind::Error;

    switch (node->kind) {
        case ASTKind::IntLiteral:
            node->type = TypeKind::Int;
            return node->type;
        case ASTKind::BoolLiteral:
            node->type = TypeKind::Bool;
            return node->type;
        case ASTKind::Identifier: {
            Symbol* s = lookup(node->text);
            if (!s) {
                error(node->loc, "variable '" + node->text + "' is not declared");
                node->type = TypeKind::Error;
            } else {
                node->type = s->type;
            }
            return node->type;
        }
        case ASTKind::UnaryExpr: {
            TypeKind inner = analyzeExpr(node->children[0].get());
            if (node->text == "-") {
                if (inner != TypeKind::Int && inner != TypeKind::Error) {
                    error(node->loc, "unary '-' expects int");
                }
                node->type = inner == TypeKind::Error ? TypeKind::Error : TypeKind::Int;
            } else {
                if (inner != TypeKind::Bool && inner != TypeKind::Error) {
                    error(node->loc, "'!' expects bool");
                }
                node->type = inner == TypeKind::Error ? TypeKind::Error : TypeKind::Bool;
            }
            return node->type;
        }
        case ASTKind::BinaryExpr: {
            TypeKind left = analyzeExpr(node->children[0].get());
            TypeKind right = analyzeExpr(node->children[1].get());
            const std::string& op = node->text;

            if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
                if ((left != TypeKind::Int || right != TypeKind::Int) && left != TypeKind::Error && right != TypeKind::Error) {
                    error(node->loc, "operator '" + op + "' expects int operands");
                }
                node->type = (left == TypeKind::Error || right == TypeKind::Error) ? TypeKind::Error : TypeKind::Int;
            } else if (op == "<" || op == "<=" || op == ">" || op == ">=") {
                if ((left != TypeKind::Int || right != TypeKind::Int) && left != TypeKind::Error && right != TypeKind::Error) {
                    error(node->loc, "operator '" + op + "' expects int operands");
                }
                node->type = (left == TypeKind::Error || right == TypeKind::Error) ? TypeKind::Error : TypeKind::Bool;
            } else if (op == "==" || op == "!=") {
                if (left != right && left != TypeKind::Error && right != TypeKind::Error) {
                    error(node->loc, "equality operands should have same type");
                }
                node->type = (left == TypeKind::Error || right == TypeKind::Error) ? TypeKind::Error : TypeKind::Bool;
            } else {
                if ((left != TypeKind::Bool || right != TypeKind::Bool) && left != TypeKind::Error && right != TypeKind::Error) {
                    error(node->loc, "operator '" + op + "' expects bool operands");
                }
                node->type = (left == TypeKind::Error || right == TypeKind::Error) ? TypeKind::Error : TypeKind::Bool;
            }
            return node->type;
        }
        default:
            return TypeKind::Error;
    }
}

} // namespace minic
