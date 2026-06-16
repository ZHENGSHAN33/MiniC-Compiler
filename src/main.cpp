#include "common.hpp"

#include "lexer.hpp"
#include "parser.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <unordered_map>

using namespace minic;

namespace {

std::string readFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open input file: " + path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}


std::string typeName(TypeKind type) {
    switch (type) {
        case TypeKind::Int: return "int";
        case TypeKind::Bool: return "bool";
        case TypeKind::Void: return "void";
        case TypeKind::Error: return "error";
    }
    return "error";
}

bool isIntegerText(const std::string& s) {
    if (s.empty()) return false;
    size_t i = (s[0] == '-') ? 1 : 0;
    if (i == s.size()) return false;
    for (; i < s.size(); ++i) if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    return true;
}


class Semantic {
public:
    void analyze(ASTNode* root) {
        enterScope();
        for (auto& child : root->children) analyzeFunction(child.get());
        exitScope();
    }

    const std::vector<CompileError>& errors() const { return errors_; }

    void printSymbols(std::ostream& out) const {
        out << "Scope  Kind      Type   Name\n";
        for (const auto& s : allSymbols_) {
            out << std::setw(5) << s.scopeLevel << "  "
                << std::setw(8) << (s.kind == SymbolKind::Function ? "function" : "variable") << "  "
                << std::setw(5) << typeName(s.type) << "  " << s.name << "\n";
        }
    }

private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes_;
    std::vector<Symbol> allSymbols_;
    std::vector<CompileError> errors_;
    TypeKind currentReturn_ = TypeKind::Void;
    int loopDepth_ = 0;

    void enterScope() { scopes_.push_back({}); }
    void exitScope() { scopes_.pop_back(); }
    int scopeLevel() const { return static_cast<int>(scopes_.size()) - 1; }

    void error(SourceLocation loc, const std::string& message) {
        errors_.push_back({"SemanticError", loc, message});
    }

    bool declare(const Symbol& symbol) {
        auto& scope = scopes_.back();
        if (scope.count(symbol.name)) return false;
        scope[symbol.name] = symbol;
        allSymbols_.push_back(symbol);
        return true;
    }

    Symbol* lookup(const std::string& name) {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) return &found->second;
        }
        return nullptr;
    }

    void analyzeFunction(ASTNode* fn) {
        Symbol symbol{fn->text, SymbolKind::Function, fn->type, scopeLevel(), fn->loc};
        if (!declare(symbol)) error(fn->loc, "function '" + fn->text + "' is already declared");
        currentReturn_ = fn->type;
        analyzeBlock(fn->children[0].get(), false);
    }

    void analyzeBlock(ASTNode* block, bool nested = true) {
        if (nested) enterScope();
        for (auto& child : block->children) analyzeStmt(child.get());
        if (nested) exitScope();
    }

    void analyzeStmt(ASTNode* node) {
        switch (node->kind) {
            case ASTKind::Block: analyzeBlock(node); break;
            case ASTKind::VarDecl: {
                Symbol symbol{node->text, SymbolKind::Variable, node->type, scopeLevel(), node->loc};
                if (!declare(symbol)) error(node->loc, "variable '" + node->text + "' is already declared in this scope");
                break;
            }
            case ASTKind::AssignStmt: {
                Symbol* s = lookup(node->text);
                TypeKind rhs = analyzeExpr(node->children[0].get());
                if (!s) error(node->loc, "variable '" + node->text + "' is not declared");
                else if (rhs != TypeKind::Error && s->type != rhs) {
                    error(node->loc, "cannot assign " + typeName(rhs) + " to " + typeName(s->type) + " variable '" + node->text + "'");
                }
                break;
            }
            case ASTKind::ReadStmt: {
                ASTNode* id = node->children[0].get();
                Symbol* s = lookup(id->text);
                if (!s) error(id->loc, "variable '" + id->text + "' is not declared");
                else if (s->type != TypeKind::Int) error(id->loc, "read currently supports int variables only");
                break;
            }
            case ASTKind::WriteStmt:
                analyzeExpr(node->children[0].get());
                break;
            case ASTKind::IfStmt: {
                TypeKind cond = analyzeExpr(node->children[0].get());
                if (cond != TypeKind::Bool && cond != TypeKind::Error) error(node->children[0]->loc, "if condition must be bool");
                analyzeStmt(node->children[1].get());
                if (node->children.size() > 2) analyzeStmt(node->children[2].get());
                break;
            }
            case ASTKind::WhileStmt: {
                TypeKind cond = analyzeExpr(node->children[0].get());
                if (cond != TypeKind::Bool && cond != TypeKind::Error) error(node->children[0]->loc, "while condition must be bool");
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
                TypeKind actual = analyzeExpr(node->children[0].get());
                if (actual != TypeKind::Error && actual != currentReturn_) error(node->loc, "return type should be " + typeName(currentReturn_));
                break;
            }
            default:
                break;
        }
    }

    TypeKind analyzeExpr(ASTNode* node) {
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
                    if (inner != TypeKind::Int && inner != TypeKind::Error) error(node->loc, "unary '-' expects int");
                    node->type = inner == TypeKind::Error ? TypeKind::Error : TypeKind::Int;
                } else {
                    if (inner != TypeKind::Bool && inner != TypeKind::Error) error(node->loc, "'!' expects bool");
                    node->type = inner == TypeKind::Error ? TypeKind::Error : TypeKind::Bool;
                }
                return node->type;
            }
            case ASTKind::BinaryExpr: {
                TypeKind left = analyzeExpr(node->children[0].get());
                TypeKind right = analyzeExpr(node->children[1].get());
                const std::string& op = node->text;
                if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
                    if ((left != TypeKind::Int || right != TypeKind::Int) && left != TypeKind::Error && right != TypeKind::Error)
                        error(node->loc, "operator '" + op + "' expects int operands");
                    node->type = (left == TypeKind::Error || right == TypeKind::Error) ? TypeKind::Error : TypeKind::Int;
                } else if (op == "<" || op == "<=" || op == ">" || op == ">=") {
                    if ((left != TypeKind::Int || right != TypeKind::Int) && left != TypeKind::Error && right != TypeKind::Error)
                        error(node->loc, "operator '" + op + "' expects int operands");
                    node->type = (left == TypeKind::Error || right == TypeKind::Error) ? TypeKind::Error : TypeKind::Bool;
                } else if (op == "==" || op == "!=") {
                    if (left != right && left != TypeKind::Error && right != TypeKind::Error) error(node->loc, "equality operands should have same type");
                    node->type = (left == TypeKind::Error || right == TypeKind::Error) ? TypeKind::Error : TypeKind::Bool;
                } else {
                    if ((left != TypeKind::Bool || right != TypeKind::Bool) && left != TypeKind::Error && right != TypeKind::Error)
                        error(node->loc, "operator '" + op + "' expects bool operands");
                    node->type = (left == TypeKind::Error || right == TypeKind::Error) ? TypeKind::Error : TypeKind::Bool;
                }
                return node->type;
            }
            default:
                return TypeKind::Error;
        }
    }
};

IROp opFromText(const std::string& op) {
    if (op == "+") return IROp::ADD;
    if (op == "-") return IROp::SUB;
    if (op == "*") return IROp::MUL;
    if (op == "/") return IROp::DIV;
    if (op == "%") return IROp::MOD;
    if (op == "<") return IROp::LT;
    if (op == "<=") return IROp::LE;
    if (op == ">") return IROp::GT;
    if (op == ">=") return IROp::GE;
    if (op == "==") return IROp::EQ;
    if (op == "!=") return IROp::NE;
    if (op == "&&") return IROp::AND;
    if (op == "||") return IROp::OR;
    return IROp::ADD;
}

std::string irOpName(IROp op) {
    switch (op) {
        case IROp::ADD: return "+";
        case IROp::SUB: return "-";
        case IROp::MUL: return "*";
        case IROp::DIV: return "/";
        case IROp::MOD: return "%";
        case IROp::ASSIGN: return "=";
        case IROp::LT: return "<";
        case IROp::LE: return "<=";
        case IROp::GT: return ">";
        case IROp::GE: return ">=";
        case IROp::EQ: return "==";
        case IROp::NE: return "!=";
        case IROp::AND: return "&&";
        case IROp::OR: return "||";
        case IROp::NOT: return "!";
        case IROp::NEG: return "neg";
        case IROp::LABEL: return "label";
        case IROp::GOTO: return "goto";
        case IROp::IF_FALSE: return "if_false";
        case IROp::READ: return "read";
        case IROp::WRITE: return "write";
        case IROp::RETURN: return "return";
    }
    return "?";
}

class IRGenerator {
public:
    IRList generate(ASTNode* root) {
        for (auto& fn : root->children) genBlock(fn->children[0].get());
        return ir_;
    }

private:
    IRList ir_;
    int tempId_ = 0;
    int labelId_ = 0;
    std::vector<std::pair<std::string, std::string>> loopLabels_;

    std::string newTemp() { return "t" + std::to_string(++tempId_); }
    std::string newLabel() { return "L" + std::to_string(++labelId_); }
    void emit(IROp op, std::string a = "", std::string b = "", std::string r = "") {
        ir_.push_back({op, std::move(a), std::move(b), std::move(r)});
    }

    void genBlock(ASTNode* block) {
        for (auto& child : block->children) genStmt(child.get());
    }

    void genStmt(ASTNode* node) {
        switch (node->kind) {
            case ASTKind::Block: genBlock(node); break;
            case ASTKind::VarDecl: break;
            case ASTKind::AssignStmt:
                emit(IROp::ASSIGN, genExpr(node->children[0].get()), "", node->text);
                break;
            case ASTKind::ReadStmt:
                emit(IROp::READ, "", "", node->children[0]->text);
                break;
            case ASTKind::WriteStmt:
                emit(IROp::WRITE, genExpr(node->children[0].get()));
                break;
            case ASTKind::ReturnStmt:
                emit(IROp::RETURN, genExpr(node->children[0].get()));
                break;
            case ASTKind::IfStmt: {
                std::string elseLabel = newLabel();
                std::string endLabel = newLabel();
                emit(IROp::IF_FALSE, genExpr(node->children[0].get()), "", elseLabel);
                genStmt(node->children[1].get());
                if (node->children.size() > 2) {
                    emit(IROp::GOTO, "", "", endLabel);
                    emit(IROp::LABEL, "", "", elseLabel);
                    genStmt(node->children[2].get());
                    emit(IROp::LABEL, "", "", endLabel);
                } else {
                    emit(IROp::LABEL, "", "", elseLabel);
                }
                break;
            }
            case ASTKind::WhileStmt: {
                std::string begin = newLabel();
                std::string end = newLabel();
                loopLabels_.push_back({begin, end});
                emit(IROp::LABEL, "", "", begin);
                emit(IROp::IF_FALSE, genExpr(node->children[0].get()), "", end);
                genStmt(node->children[1].get());
                emit(IROp::GOTO, "", "", begin);
                emit(IROp::LABEL, "", "", end);
                loopLabels_.pop_back();
                break;
            }
            case ASTKind::BreakStmt:
                if (!loopLabels_.empty()) emit(IROp::GOTO, "", "", loopLabels_.back().second);
                break;
            case ASTKind::ContinueStmt:
                if (!loopLabels_.empty()) emit(IROp::GOTO, "", "", loopLabels_.back().first);
                break;
            default:
                break;
        }
    }

    std::string genExpr(ASTNode* node) {
        switch (node->kind) {
            case ASTKind::IntLiteral: return node->text;
            case ASTKind::BoolLiteral: return node->text == "true" ? "1" : "0";
            case ASTKind::Identifier: return node->text;
            case ASTKind::UnaryExpr: {
                std::string value = genExpr(node->children[0].get());
                std::string temp = newTemp();
                emit(node->text == "!" ? IROp::NOT : IROp::NEG, value, "", temp);
                return temp;
            }
            case ASTKind::BinaryExpr: {
                std::string left = genExpr(node->children[0].get());
                std::string right = genExpr(node->children[1].get());
                std::string temp = newTemp();
                emit(opFromText(node->text), left, right, temp);
                return temp;
            }
            default:
                return "0";
        }
    }
};

int calc(IROp op, int a, int b) {
    switch (op) {
        case IROp::ADD: return a + b;
        case IROp::SUB: return a - b;
        case IROp::MUL: return a * b;
        case IROp::DIV: return b == 0 ? 0 : a / b;
        case IROp::MOD: return b == 0 ? 0 : a % b;
        case IROp::LT: return a < b;
        case IROp::LE: return a <= b;
        case IROp::GT: return a > b;
        case IROp::GE: return a >= b;
        case IROp::EQ: return a == b;
        case IROp::NE: return a != b;
        case IROp::AND: return a && b;
        case IROp::OR: return a || b;
        default: return 0;
    }
}

IRList optimizeIR(const IRList& input) {
    IRList out;
    std::unordered_map<std::string, std::string> constants;
    bool reachable = true;
    for (const auto& q : input) {
        if (!reachable && q.op != IROp::LABEL) continue;
        Quad next = q;
        if (q.op == IROp::LABEL) {
            reachable = true;
            constants.clear();
        }
        if (q.op == IROp::GOTO || q.op == IROp::IF_FALSE) {
            constants.clear();
        }
        if (constants.count(next.arg1)) next.arg1 = constants[next.arg1];
        if (constants.count(next.arg2)) next.arg2 = constants[next.arg2];

        bool binary = q.op == IROp::ADD || q.op == IROp::SUB || q.op == IROp::MUL || q.op == IROp::DIV ||
                      q.op == IROp::MOD || q.op == IROp::LT || q.op == IROp::LE || q.op == IROp::GT ||
                      q.op == IROp::GE || q.op == IROp::EQ || q.op == IROp::NE || q.op == IROp::AND || q.op == IROp::OR;
        if (binary && isIntegerText(next.arg1) && isIntegerText(next.arg2)) {
            next.op = IROp::ASSIGN;
            next.arg1 = std::to_string(calc(q.op, std::stoi(next.arg1), std::stoi(next.arg2)));
            next.arg2.clear();
            constants[next.result] = next.arg1;
        } else if (next.op == IROp::NEG && isIntegerText(next.arg1)) {
            next.op = IROp::ASSIGN;
            next.arg1 = std::to_string(-std::stoi(next.arg1));
            constants[next.result] = next.arg1;
        } else if (next.op == IROp::NOT && isIntegerText(next.arg1)) {
            next.op = IROp::ASSIGN;
            next.arg1 = std::stoi(next.arg1) ? "0" : "1";
            constants[next.result] = next.arg1;
        } else if (next.op == IROp::ASSIGN && isIntegerText(next.arg1)) {
            constants[next.result] = next.arg1;
        } else if (!next.result.empty()) {
            constants.erase(next.result);
        }

        out.push_back(next);
        if (q.op == IROp::GOTO || q.op == IROp::RETURN) reachable = false;
    }
    return out;
}

void printIR(std::ostream& out, const IRList& ir) {
    for (const auto& q : ir) {
        switch (q.op) {
            case IROp::LABEL: out << q.result << ":\n"; break;
            case IROp::GOTO: out << "  goto " << q.result << "\n"; break;
            case IROp::IF_FALSE: out << "  if_false " << q.arg1 << " goto " << q.result << "\n"; break;
            case IROp::ASSIGN: out << "  " << q.result << " = " << q.arg1 << "\n"; break;
            case IROp::READ: out << "  read " << q.result << "\n"; break;
            case IROp::WRITE: out << "  write " << q.arg1 << "\n"; break;
            case IROp::RETURN: out << "  return " << q.arg1 << "\n"; break;
            case IROp::NOT:
            case IROp::NEG: out << "  " << q.result << " = " << irOpName(q.op) << " " << q.arg1 << "\n"; break;
            default: out << "  " << q.result << " = " << q.arg1 << " " << irOpName(q.op) << " " << q.arg2 << "\n"; break;
        }
    }
}

std::vector<VMInst> generateVM(const IRList& ir) {
    std::vector<VMInst> code;
    auto load = [&](const std::string& x) { code.push_back({isIntegerText(x) ? "PUSH" : "LOAD", x}); };
    for (const auto& q : ir) {
        switch (q.op) {
            case IROp::LABEL: code.push_back({"LABEL", q.result}); break;
            case IROp::GOTO: code.push_back({"JMP", q.result}); break;
            case IROp::IF_FALSE: load(q.arg1); code.push_back({"JZ", q.result}); break;
            case IROp::ASSIGN: load(q.arg1); code.push_back({"STORE", q.result}); break;
            case IROp::READ: code.push_back({"READ", q.result}); break;
            case IROp::WRITE: load(q.arg1); code.push_back({"PRINT", ""}); break;
            case IROp::RETURN: load(q.arg1); code.push_back({"RET", ""}); break;
            case IROp::NOT: load(q.arg1); code.push_back({"NOT", ""}); code.push_back({"STORE", q.result}); break;
            case IROp::NEG: load(q.arg1); code.push_back({"NEG", ""}); code.push_back({"STORE", q.result}); break;
            default:
                load(q.arg1);
                load(q.arg2);
                code.push_back({irOpName(q.op), ""});
                code.push_back({"STORE", q.result});
                break;
        }
    }
    code.push_back({"HALT", ""});
    return code;
}

void printVM(std::ostream& out, const std::vector<VMInst>& code) {
    for (const auto& inst : code) {
        out << inst.op;
        if (!inst.arg.empty()) out << " " << inst.arg;
        out << "\n";
    }
}

int runVM(const std::vector<VMInst>& code, std::istream& in, std::ostream& out) {
    std::unordered_map<std::string, int> vars;
    std::unordered_map<std::string, size_t> labels;
    std::vector<int> stack;
    for (size_t i = 0; i < code.size(); ++i) if (code[i].op == "LABEL") labels[code[i].arg] = i;

    auto pop = [&]() {
        int v = stack.empty() ? 0 : stack.back();
        if (!stack.empty()) stack.pop_back();
        return v;
    };

    for (size_t pc = 0; pc < code.size(); ++pc) {
        const auto& inst = code[pc];
        if (inst.op == "PUSH") stack.push_back(std::stoi(inst.arg));
        else if (inst.op == "LOAD") stack.push_back(vars[inst.arg]);
        else if (inst.op == "STORE") vars[inst.arg] = pop();
        else if (inst.op == "READ") {
            int v = 0;
            in >> v;
            vars[inst.arg] = v;
        } else if (inst.op == "PRINT") out << pop() << "\n";
        else if (inst.op == "+") { int b = pop(), a = pop(); stack.push_back(a + b); }
        else if (inst.op == "-") { int b = pop(), a = pop(); stack.push_back(a - b); }
        else if (inst.op == "*") { int b = pop(), a = pop(); stack.push_back(a * b); }
        else if (inst.op == "/") { int b = pop(), a = pop(); stack.push_back(b == 0 ? 0 : a / b); }
        else if (inst.op == "%") { int b = pop(), a = pop(); stack.push_back(b == 0 ? 0 : a % b); }
        else if (inst.op == "<") { int b = pop(), a = pop(); stack.push_back(a < b); }
        else if (inst.op == "<=") { int b = pop(), a = pop(); stack.push_back(a <= b); }
        else if (inst.op == ">") { int b = pop(), a = pop(); stack.push_back(a > b); }
        else if (inst.op == ">=") { int b = pop(), a = pop(); stack.push_back(a >= b); }
        else if (inst.op == "==") { int b = pop(), a = pop(); stack.push_back(a == b); }
        else if (inst.op == "!=") { int b = pop(), a = pop(); stack.push_back(a != b); }
        else if (inst.op == "&&") { int b = pop(), a = pop(); stack.push_back(a && b); }
        else if (inst.op == "||") { int b = pop(), a = pop(); stack.push_back(a || b); }
        else if (inst.op == "NOT") stack.push_back(!pop());
        else if (inst.op == "NEG") stack.push_back(-pop());
        else if (inst.op == "JMP") pc = labels[inst.arg];
        else if (inst.op == "JZ") {
            if (!pop()) pc = labels[inst.arg];
        } else if (inst.op == "RET") return pop();
        else if (inst.op == "HALT") return 0;
    }
    return 0;
}

std::string generatePseudoX86(const IRList& ir) {
    std::ostringstream out;
    out << "; Mini-C pseudo x86-64 output for presentation\n";
    for (const auto& q : ir) {
        if (q.op == IROp::LABEL) out << q.result << ":\n";
        else if (q.op == IROp::GOTO) out << "  jmp " << q.result << "\n";
        else if (q.op == IROp::IF_FALSE) out << "  cmp [" << q.arg1 << "], 0\n  je " << q.result << "\n";
        else if (q.op == IROp::ASSIGN) out << "  mov [" << q.result << "], " << q.arg1 << "\n";
        else if (q.op == IROp::READ) out << "  call read_int ; -> " << q.result << "\n";
        else if (q.op == IROp::WRITE) out << "  call print_int ; " << q.arg1 << "\n";
        else if (q.op == IROp::RETURN) out << "  mov rax, " << q.arg1 << "\n  ret\n";
        else out << "  ; " << q.result << " = " << q.arg1 << " " << irOpName(q.op) << " " << q.arg2 << "\n";
    }
    return out.str();
}

struct Pipeline {
    TokenList tokens;
    std::unique_ptr<ASTNode> ast;
    Semantic semantic;
    IRList ir;
    IRList optIr;
};

bool printErrors(const std::vector<CompileError>& errors) {
    for (const auto& e : errors) std::cerr << e.str() << "\n";
    return !errors.empty();
}

Pipeline compileFrontend(const std::string& source) {
    Pipeline p;
    Lexer lexer(source);
    p.tokens = lexer.scan();
    if (printErrors(lexer.errors())) throw std::runtime_error("lexical analysis failed");

    Parser parser(p.tokens);
    p.ast = parser.parseProgram();
    if (printErrors(parser.errors())) throw std::runtime_error("syntax analysis failed");

    p.semantic.analyze(p.ast.get());
    if (printErrors(p.semantic.errors())) throw std::runtime_error("semantic analysis failed");

    IRGenerator gen;
    p.ir = gen.generate(p.ast.get());
    p.optIr = optimizeIR(p.ir);
    return p;
}

void help() {
    std::cout
        << "Mini-C Compiler\n"
        << "Usage:\n"
        << "  compiler <input.mc> [--tokens|--ast|--dot|--check|--ir|--vm|--run|-S] [--opt]\n"
        << "Options:\n"
        << "  --tokens   print token stream\n"
        << "  --ast      print AST tree\n"
        << "  --dot      print Graphviz DOT for AST\n"
        << "  --check    run semantic check and print symbol table\n"
        << "  --ir       print three-address IR; add --opt for optimized IR\n"
        << "  --vm       print stack VM code\n"
        << "  --run      compile and run on VM\n"
        << "  -S         print presentation-friendly pseudo x86-64 assembly\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        help();
        return argc < 2 ? 1 : 0;
    }

    std::string path = argv[1];
    std::set<std::string> options;
    for (int i = 2; i < argc; ++i) options.insert(argv[i]);

    try {
        std::string source = readFile(path);
        bool onlyTokens = options.count("--tokens") > 0;

        if (onlyTokens) {
            Lexer lexer(source);
            TokenList tokens = lexer.scan();
            printTokens(std::cout, tokens);
            return printErrors(lexer.errors()) ? 1 : 0;
        }

        Pipeline p = compileFrontend(source);

        if (options.count("--ast")) {
            printAST(std::cout, p.ast.get());
        } else if (options.count("--dot")) {
            DotPrinter printer;
            std::cout << printer.print(p.ast.get());
        } else if (options.count("--check")) {
            std::cout << "semantic check passed\n";
            p.semantic.printSymbols(std::cout);
        } else if (options.count("--ir")) {
            printIR(std::cout, options.count("--opt") ? p.optIr : p.ir);
        } else if (options.count("--vm")) {
            auto vm = generateVM(options.count("--opt") ? p.optIr : p.ir);
            printVM(std::cout, vm);
        } else if (options.count("--run")) {
            auto vm = generateVM(p.optIr);
            return runVM(vm, std::cin, std::cout);
        } else if (options.count("-S")) {
            std::cout << generatePseudoX86(p.optIr);
        } else {
            std::cout << "compile passed. Use --help to see display options.\n";
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
    return 0;
}
