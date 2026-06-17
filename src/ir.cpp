#include "ir.hpp"
#include <unordered_map>
#include <cctype>
namespace minic
{

    // ==================== IRGenerator 实现 ====================
    IRList IRGenerator::generate(ASTNode *root)
    {
        ir_.clear();
        tempId_ = 0;
        labelId_ = 0;
        loopLabels_.clear();
        // 遍历顶层所有函数
        for (auto &fn : root->children)
        {
            genBlock(fn->children[0].get());
        }
        return ir_;
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

    void IRGenerator::genBlock(ASTNode *block)
    {
        for (auto &child : block->children)
        {
            genStmt(child.get());
        }
    }

    void IRGenerator::genStmt(ASTNode *node)
    {
        switch (node->kind)
        {
        case ASTKind::Block:
            genBlock(node);
            break;
        case ASTKind::VarDecl:
            // 变量声明仅符号表记录，无IR指令
            break;
        case ASTKind::AssignStmt:
        {
            std::string rhs = genExpr(node->children[0].get());
            emit(IROp::ASSIGN, rhs, "", node->text);
            break;
        }
        case ASTKind::ReadStmt:
            emit(IROp::READ, "", "", node->children[0]->text);
            break;
        case ASTKind::WriteStmt:
        {
            std::string val = genExpr(node->children[0].get());
            emit(IROp::WRITE, val);
            break;
        }
        case ASTKind::ReturnStmt:
        {
            std::string retVal = genExpr(node->children[0].get());
            emit(IROp::RETURN, retVal);
            break;
        }
        case ASTKind::IfStmt:
        {
            std::string elseLabel = newLabel();
            std::string endLabel = newLabel();
            // 条件为假跳else
            emit(IROp::IF_FALSE, genExpr(node->children[0].get()), "", elseLabel);
            // if分支语句
            genStmt(node->children[1].get());
            // 存在else分支
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
            std::string beginLbl = newLabel();
            std::string endLbl = newLabel();
            loopLabels_.push_back({beginLbl, endLbl});
            emit(IROp::LABEL, "", "", beginLbl);
            // 条件不成立跳出循环
            emit(IROp::IF_FALSE, genExpr(node->children[0].get()), "", endLbl);
            genStmt(node->children[1].get());
            emit(IROp::GOTO, "", "", beginLbl);
            emit(IROp::LABEL, "", "", endLbl);
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
            std::string sub = genExpr(node->children[0].get());
            std::string tmp = newTemp();
            if (node->text == "!")
            {
                emit(IROp::NOT, sub, "", tmp);
            }
            else
            {
                emit(IROp::NEG, sub, "", tmp);
            }
            return tmp;
        }
        case ASTKind::BinaryExpr:
        {
            std::string left = genExpr(node->children[0].get());
            std::string right = genExpr(node->children[1].get());
            std::string tmp = newTemp();
            IROp op = opFromText(node->text);
            emit(op, left, right, tmp);
            return tmp;
        }
        default:
            return "0";
        }
    }

    // ==================== IR工具辅助函数 ====================
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

    bool isIntegerText(const std::string &s)
    {
        if (s.empty())
            return false;
        size_t i = (s[0] == '-') ? 1 : 0;
        if (i == s.size())
            return false;
        for (; i < s.size(); ++i)
        {
            if (!std::isdigit(static_cast<unsigned char>(s[i])))
                return false;
        }
        return true;
    }

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

    // ==================== IR优化器 ====================
    IRList optimizeIR(const IRList &input)
    {
        IRList out;
        // 常量传播映射：临时变量 -> 常量字符串
        std::unordered_map<std::string, std::string> constants;
        bool reachable = true;

        for (const auto &q : input)
        {
            // 不可达代码直接跳过（label除外）
            if (!reachable && q.op != IROp::LABEL)
                continue;

            Quad next = q;
            // 遇到标签，重置可达标记与常量表
            if (q.op == IROp::LABEL)
            {
                reachable = true;
                constants.clear();
            }
            // 跳转指令清空常量表
            if (q.op == IROp::GOTO || q.op == IROp::IF_FALSE)
            {
                constants.clear();
            }

            // 常量传播：把arg1/arg2替换为已知常量
            if (constants.count(next.arg1))
                next.arg1 = constants[next.arg1];
            if (constants.count(next.arg2))
                next.arg2 = constants[next.arg2];

            // 判断是否二元算术/比较逻辑运算
            bool binaryOp = q.op == IROp::ADD || q.op == IROp::SUB || q.op == IROp::MUL || q.op == IROp::DIV || q.op == IROp::MOD || q.op == IROp::LT || q.op == IROp::LE || q.op == IROp::GT || q.op == IROp::GE || q.op == IROp::EQ || q.op == IROp::NE || q.op == IROp::AND || q.op == IROp::OR;

            // 常量折叠：两个操作数都是数字直接计算结果
            if (binaryOp && isIntegerText(next.arg1) && isIntegerText(next.arg2))
            {
                int val = calc(q.op, std::stoi(next.arg1), std::stoi(next.arg2));
                next.op = IROp::ASSIGN;
                next.arg1 = std::to_string(val);
                next.arg2.clear();
                constants[next.result] = next.arg1;
            }
            // 一元负号常量折叠
            else if (next.op == IROp::NEG && isIntegerText(next.arg1))
            {
                next.op = IROp::ASSIGN;
                next.arg1 = std::to_string(-std::stoi(next.arg1));
                constants[next.result] = next.arg1;
            }
            // 逻辑非常量折叠
            else if (next.op == IROp::NOT && isIntegerText(next.arg1))
            {
                next.op = IROp::ASSIGN;
                next.arg1 = std::stoi(next.arg1) ? "0" : "1";
                constants[next.result] = next.arg1;
            }
            // 普通常量赋值，存入传播表
            else if (next.op == IROp::ASSIGN && isIntegerText(next.arg1))
            {
                constants[next.result] = next.arg1;
            }
            // 变量被重新赋值，删除原有常量记录
            else if (!next.result.empty())
            {
                constants.erase(next.result);
            }

            out.push_back(next);
            // goto/return后代码不可达
            if (q.op == IROp::GOTO || q.op == IROp::RETURN)
                reachable = false;
        }
        return out;
    }

    // ==================== IR打印函数 ====================
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