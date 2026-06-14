#include "../include/ir.hpp"
#include "../include/common.hpp"
#include <unordered_map>
#include <string>
#include <cctype>

namespace minic
{

    IROp opFromText(const std::string &op)
    {
        if (op == "+")
            return IROp::ADD;
        if (op == "-")
            return IROp::SUB;
        if (op == "*")
            return IROp::MUL;
        if (op == "/")
            return IROp::DIV;
        if (op == "%")
            return IROp::MOD;
        if (op == "<")
            return IROp::LT;
        if (op == "<=")
            return IROp::LE;
        if (op == ">")
            return IROp::GT;
        if (op == ">=")
            return IROp::GE;
        if (op == "==")
            return IROp::EQ;
        if (op == "!=")
            return IROp::NE;
        if (op == "&&")
            return IROp::AND;
        if (op == "||")
            return IROp::OR;
        return IROp::ADD;
    }

    std::string irOpName(IROp op)
    {
        switch (op)
        {
        case IROp::ADD:
            return "+";
        case IROp::SUB:
            return "-";
        case IROp::MUL:
            return "*";
        case IROp::DIV:
            return "/";
        case IROp::MOD:
            return "%";
        case IROp::ASSIGN:
            return "=";
        case IROp::LT:
            return "<";
        case IROp::LE:
            return "<=";
        case IROp::GT:
            return ">";
        case IROp::GE:
            return ">=";
        case IROp::EQ:
            return "==";
        case IROp::NE:
            return "!=";
        case IROp::AND:
            return "&&";
        case IROp::OR:
            return "||";
        case IROp::NOT:
            return "!";
        case IROp::NEG:
            return "neg";
        case IROp::LABEL:
            return "label";
        case IROp::GOTO:
            return "goto";
        case IROp::IF_FALSE:
            return "if_false";
        case IROp::READ:
            return "read";
        case IROp::WRITE:
            return "write";
        case IROp::RETURN:
            return "return";
        }
        return "?";
    }

    int calc(IROp op, int a, int b)
    {
        switch (op)
        {
        case IROp::ADD:
            return a + b;
        case IROp::SUB:
            return a - b;
        case IROp::MUL:
            return a * b;
        case IROp::DIV:
            return b == 0 ? 0 : a / b;
        case IROp::MOD:
            return b == 0 ? 0 : a % b;
        case IROp::LT:
            return a < b;
        case IROp::LE:
            return a <= b;
        case IROp::GT:
            return a > b;
        case IROp::GE:
            return a >= b;
        case IROp::EQ:
            return a == b;
        case IROp::NE:
            return a != b;
        case IROp::AND:
            return a && b;
        case IROp::OR:
            return a || b;
        default:
            return 0;
        }
    }

    std::string IRGenerator::newTemp()
    {
        return "t" + std::to_string(++tempId_);
    }

    std::string IRGenerator::newLabel()
    {
        return "L" + std::to_string(++labelId_);
    }

    void IRGenerator::emit(IROp op, std::string a, std::string b, std::string r)
    {
        ir_.push_back({op, std::move(a), std::move(b), std::move(r)});
    }

    IRList IRGenerator::generate(ASTNode *root)
    {
        if (!root)
            return ir_;
        for (auto &fn : root->children)
        {
            genBlock(fn->children[0].get());
        }
        return ir_;
    }

    void IRGenerator::genBlock(ASTNode *block)
    {
        if (!block)
            return;
        for (auto &child : block->children)
        {
            genStmt(child.get());
        }
    }

    void IRGenerator::genStmt(ASTNode *node)
    {
        if (!node)
            return;
        switch (node->kind)
        {
        case ASTKind::Block:
            genBlock(node);
            break;
        case ASTKind::VarDecl:
            break;
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
        case ASTKind::IfStmt:
        {
            std::string elseLabel = newLabel();
            std::string endLabel = newLabel();
            emit(IROp::IF_FALSE, genExpr(node->children[0].get()), "", elseLabel);
            genStmt(node->children[1].get());
            if (node->children.size() > 2)
            {
                emit(IROp::GOTO, "", "", endLabel);
                emit(IROp::LABEL, "", "", elseLabel);
                genStmt(node->children[2].get());
                emit(IROp::LABEL, "", "", endLabel);
            }
            else
            {
                emit(IROp::LABEL, "", "", elseLabel);
            }
            break;
        }
        case ASTKind::WhileStmt:
        {
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
            if (!loopLabels_.empty())
            {
                emit(IROp::GOTO, "", "", loopLabels_.back().second);
            }
            break;
        case ASTKind::ContinueStmt:
            if (!loopLabels_.empty())
            {
                emit(IROp::GOTO, "", "", loopLabels_.back().first);
            }
            break;
        default:
            break;
        }
    }

    std::string IRGenerator::genExpr(ASTNode *node)
    {
        if (!node)
            return "0";
        switch (node->kind)
        {
        case ASTKind::IntLiteral:
            return node->text;
        case ASTKind::BoolLiteral:
            return node->text == "true" ? "1" : "0";
        case ASTKind::Identifier:
            return node->text;
        case ASTKind::UnaryExpr:
        {
            std::string value = genExpr(node->children[0].get());
            std::string temp = newTemp();
            emit(node->text == "!" ? IROp::NOT : IROp::NEG, value, "", temp);
            return temp;
        }
        case ASTKind::BinaryExpr:
        {
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

    static bool isIntegerText(const std::string &s)
    {
        if (s.empty())
            return false;
        size_t i = 0;
        if (s[0] == '-' || s[0] == '+')
            i = 1;
        for (; i < s.size(); ++i)
        {
            if (!isdigit(s[i]))
                return false;
        }
        return !s.empty();
    }

    IRList optimizeIR(const IRList &input)
    {
        IRList out;
        std::unordered_map<std::string, std::string> constants;
        bool reachable = true;
        for (const auto &q : input)
        {
            if (!reachable && q.op != IROp::LABEL)
                continue;
            Quad next = q;
            if (q.op == IROp::LABEL)
            {
                reachable = true;
                constants.clear();
            }
            if (q.op == IROp::GOTO || q.op == IROp::IF_FALSE)
            {
                constants.clear();
            }
            if (constants.count(next.arg1))
                next.arg1 = constants[next.arg1];
            if (constants.count(next.arg2))
                next.arg2 = constants[next.arg2];

            bool binary = q.op == IROp::ADD || q.op == IROp::SUB || q.op == IROp::MUL || q.op == IROp::DIV ||
                          q.op == IROp::MOD || q.op == IROp::LT || q.op == IROp::LE || q.op == IROp::GT ||
                          q.op == IROp::GE || q.op == IROp::EQ || q.op == IROp::NE || q.op == IROp::AND || q.op == IROp::OR;
            if (binary && isIntegerText(next.arg1) && isIntegerText(next.arg2))
            {
                next.op = IROp::ASSIGN;
                next.arg1 = std::to_string(calc(q.op, std::stoi(next.arg1), std::stoi(next.arg2)));
                next.arg2.clear();
                constants[next.result] = next.arg1;
            }
            else if (next.op == IROp::NEG && isIntegerText(next.arg1))
            {
                next.op = IROp::ASSIGN;
                next.arg1 = std::to_string(-std::stoi(next.arg1));
                constants[next.result] = next.arg1;
            }
            else if (next.op == IROp::NOT && isIntegerText(next.arg1))
            {
                next.op = IROp::ASSIGN;
                next.arg1 = std::stoi(next.arg1) ? "0" : "1";
                constants[next.result] = next.arg1;
            }
            else if (next.op == IROp::ASSIGN && isIntegerText(next.arg1))
            {
                constants[next.result] = next.arg1;
            }
            else if (!next.result.empty())
            {
                constants.erase(next.result);
            }
            out.push_back(next);
            if (q.op == IROp::GOTO || q.op == IROp::RETURN)
            {
                reachable = false;
            }
        }
        return out;
    }

    void printIR(std::ostream &out, const IRList &ir)
    {
        for (const auto &q : ir)
        {
            switch (q.op)
            {
            case IROp::LABEL:
                out << q.result << ":\n";
                break;
            case IROp::GOTO:
                out << "  goto " << q.result << "\n";
                break;
            case IROp::IF_FALSE:
                out << "  if_false " << q.arg1 << " goto " << q.result << "\n";
                break;
            case IROp::ASSIGN:
                out << "  " << q.result << " = " << q.arg1 << "\n";
                break;
            case IROp::READ:
                out << "  read " << q.result << "\n";
                break;
            case IROp::WRITE:
                out << "  write " << q.arg1 << "\n";
                break;
            case IROp::RETURN:
                out << "  return " << q.arg1 << "\n";
                break;
            case IROp::NOT:
            case IROp::NEG:
                out << "  " << q.result << " = " << irOpName(q.op) << " " << q.arg1 << "\n";
                break;
            default:
                out << "  " << q.result << " = " << q.arg1 << " " << irOpName(q.op) << " " << q.arg2 << "\n";
                break;
            }
        }
    }

} // namespace minic