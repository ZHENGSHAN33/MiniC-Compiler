#include "x86.hpp"
#include "ir.hpp"
#include <algorithm>
#include <cctype>
#include <set>

namespace minic
{

    void X86Generator::emit(const std::string &line)
    {
        code_.push_back(line);
    }

    void X86Generator::emitln(const std::string &line)
    {
        code_.push_back(line);
    }

    int X86Generator::getVarOffset(const std::string &name)
    {
        if (varOffsets_.find(name) == varOffsets_.end())
        {
            nextOffset_ -= 8;
            varOffsets_[name] = nextOffset_;
            stackSize_ = std::max(stackSize_, -nextOffset_);
        }
        return varOffsets_[name];
    }

    void X86Generator::allocateVars(const IRList &ir)
    {
        std::set<std::string> vars;
        for (const auto &q : ir)
        {
            if (!q.result.empty() && q.result[0] != 'L')
                vars.insert(q.result);
            if (!q.arg1.empty() && q.arg1[0] != 'L' && !isIntegerText(q.arg1))
                vars.insert(q.arg1);
            if (!q.arg2.empty() && q.arg2[0] != 'L' && !isIntegerText(q.arg2))
                vars.insert(q.arg2);
        }
        for (const auto &v : vars)
            getVarOffset(v);
    }

    bool X86Generator::isIntegerText(const std::string &s)
    {
        if (s.empty())
            return false;
        if (s[0] == '-' || s[0] == '+')
            return std::all_of(s.begin() + 1, s.end(), ::isdigit);
        return std::all_of(s.begin(), s.end(), ::isdigit);
    }

    std::string X86Generator::generate(const IRList &ir)
    {
        code_.clear();
        varOffsets_.clear();
        stackSize_ = 0;
        nextOffset_ = 0;

        allocateVars(ir);

        emit(".text");
        emit(".global _main");
        emit("_main:");
        emit("  pushq %rbp");
        emit("  movq %rsp, %rbp");
        if (stackSize_ > 0)
        {
            emit("  subq $" + std::to_string(stackSize_) + ", %rsp");
        }

        for (const auto &q : ir)
        {
            switch (q.op)
            {
            case IROp::LABEL:
                emit(q.result + ":");
                break;

            case IROp::GOTO:
                emit("  jmp " + q.result);
                break;

            case IROp::IF_FALSE:
                emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), %rax");
                emit("  testq %rax, %rax");
                emit("  jz " + q.result);
                break;

            case IROp::ASSIGN:
                if (isIntegerText(q.arg1))
                {
                    emit("  movq $" + q.arg1 + ", " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                }
                else
                {
                    emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), %rax");
                    emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                }
                break;

            case IROp::READ:
                emit("  leaq .LC1(%rip), %rdi");
                emit("  leaq " + std::to_string(getVarOffset(q.result)) + "(%rbp), %rsi");
                emit("  movq $0, %rax");
                emit("  callq _scanf");
                emit("  movl " + std::to_string(getVarOffset(q.result)) + "(%rbp), %eax");
                emit("  movslq %eax, %rax");
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::WRITE:
                if (isIntegerText(q.arg1))
                {
                    emit("  movq $" + q.arg1 + ", %rdi");
                }
                else
                {
                    emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), %rdi");
                }
                emit("  callq _print_int");
                break;

            case IROp::RETURN:
                if (isIntegerText(q.arg1))
                {
                    emit("  movq $" + q.arg1 + ", %rax");
                }
                else
                {
                    emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), %rax");
                }
                emit("  movq %rbp, %rsp");
                emit("  popq %rbp");
                emit("  retq");
                break;

            case IROp::ADD:
                if (isIntegerText(q.arg2))
                {
                    emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), %rax");
                    emit("  addq $" + q.arg2 + ", %rax");
                }
                else
                {
                    emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), %rax");
                    emit("  addq " + std::to_string(getVarOffset(q.arg2)) + "(%rbp), %rax");
                }
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::SUB:
                if (isIntegerText(q.arg2))
                {
                    emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), %rax");
                    emit("  subq $" + q.arg2 + ", %rax");
                }
                else
                {
                    emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), %rax");
                    emit("  subq " + std::to_string(getVarOffset(q.arg2)) + "(%rbp), %rax");
                }
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::MUL:
                if (isIntegerText(q.arg2))
                {
                    emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), %rax");
                    emit("  imulq $" + q.arg2 + ", %rax");
                }
                else
                {
                    emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), %rax");
                    emit("  imulq " + std::to_string(getVarOffset(q.arg2)) + "(%rbp), %rax");
                }
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::DIV:
                emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), %rax");
                emit("  cqto");
                if (isIntegerText(q.arg2))
                {
                    emit("  idivq $" + q.arg2);
                }
                else
                {
                    emit("  idivq " + std::to_string(getVarOffset(q.arg2)) + "(%rbp)");
                }
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::LT:
                emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), %rax");
                emit("  cmpq " + std::to_string(getVarOffset(q.arg2)) + "(%rbp), %rax");
                emit("  setl %al");
                emit("  movzbl %al, %eax");
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::GT:
                emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), %rax");
                emit("  cmpq " + std::to_string(getVarOffset(q.arg2)) + "(%rbp), %rax");
                emit("  setg %al");
                emit("  movzbl %al, %eax");
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::LE:
                emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), %rax");
                emit("  cmpq " + std::to_string(getVarOffset(q.arg2)) + "(%rbp), %rax");
                emit("  setle %al");
                emit("  movzbl %al, %eax");
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::GE:
                emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), %rax");
                emit("  cmpq " + std::to_string(getVarOffset(q.arg2)) + "(%rbp), %rax");
                emit("  setge %al");
                emit("  movzbl %al, %eax");
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::EQ:
                emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), %rax");
                emit("  cmpq " + std::to_string(getVarOffset(q.arg2)) + "(%rbp), %rax");
                emit("  sete %al");
                emit("  movzbl %al, %eax");
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::NE:
                emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), %rax");
                emit("  cmpq " + std::to_string(getVarOffset(q.arg2)) + "(%rbp), %rax");
                emit("  setne %al");
                emit("  movzbl %al, %eax");
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::AND:
                emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), %rax");
                emit("  andq " + std::to_string(getVarOffset(q.arg2)) + "(%rbp), %rax");
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::OR:
                emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), %rax");
                emit("  orq " + std::to_string(getVarOffset(q.arg2)) + "(%rbp), %rax");
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::NOT:
                emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), %rax");
                emit("  notq %rax");
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            default:
                break;
            }
        }

        emit(".data");
        emit("_print_buf: .space 32");
        emit("_read_buf: .space 16");
        emit(".LC0: .string \"%d\\n\"");
        emit(".LC1: .string \"%d\"");

        emit(".text");
        emit("_print_int:");
        emit("  pushq %rbp");
        emit("  movq %rsp, %rbp");
        emit("  movq %rdi, %rsi");
        emit("  leaq .LC0(%rip), %rdi");
        emit("  movq $0, %rax");
        emit("  callq _printf");
        emit("  popq %rbp");
        emit("  retq");

        std::string result;
        for (const auto &line : code_)
        {
            result += line + "\n";
        }
        return result;
    }

} // namespace minic