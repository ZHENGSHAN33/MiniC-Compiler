#include "x86.hpp"

#include <algorithm>
#include <cctype>
#include <set>

namespace minic
{

namespace
{

#if defined(_WIN32)
    constexpr bool kWindowsAbi = true;
#else
    constexpr bool kWindowsAbi = false;
#endif

#if defined(__APPLE__)
    constexpr bool kDarwinSymbols = true;
#else
    constexpr bool kDarwinSymbols = false;
#endif

    int alignTo16(int value)
    {
        return (value + 15) / 16 * 16;
    }

} // namespace

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
        size_t i = (s[0] == '-' || s[0] == '+') ? 1 : 0;
        if (i == s.size())
            return false;
        return std::all_of(s.begin() + static_cast<std::ptrdiff_t>(i), s.end(),
                           [](unsigned char ch) { return std::isdigit(ch) != 0; });
    }

    int X86Generator::alignedFrameSize() const
    {
        int frame = stackSize_;
        if (kWindowsAbi)
            frame += 32;
        return alignTo16(frame);
    }

    std::string X86Generator::symbol(const std::string &name) const
    {
        return kDarwinSymbols ? "_" + name : name;
    }

    std::string X86Generator::firstArgReg() const
    {
        return kWindowsAbi ? "%rcx" : "%rdi";
    }

    std::string X86Generator::secondArgReg() const
    {
        return kWindowsAbi ? "%rdx" : "%rsi";
    }

    void X86Generator::loadValueToRax(const std::string &value)
    {
        if (isIntegerText(value))
            emit("  movq $" + value + ", %rax");
        else
            emit("  movq " + std::to_string(getVarOffset(value)) + "(%rbp), %rax");
    }

    void X86Generator::emitCompare(const Quad &q, const std::string &setInstr)
    {
        if (isIntegerText(q.arg1) && isIntegerText(q.arg2))
        {
            int left = std::stoi(q.arg1);
            int right = std::stoi(q.arg2);
            bool value = false;
            if (setInstr == "setl")
                value = left < right;
            else if (setInstr == "setle")
                value = left <= right;
            else if (setInstr == "setg")
                value = left > right;
            else if (setInstr == "setge")
                value = left >= right;
            else if (setInstr == "sete")
                value = left == right;
            else if (setInstr == "setne")
                value = left != right;
            emit("  movq $" + std::string(value ? "1" : "0") + ", " +
                 std::to_string(getVarOffset(q.result)) + "(%rbp)");
            return;
        }

        loadValueToRax(q.arg1);
        if (isIntegerText(q.arg2))
            emit("  cmpq $" + q.arg2 + ", %rax");
        else
            emit("  cmpq " + std::to_string(getVarOffset(q.arg2)) + "(%rbp), %rax");
        emit("  " + setInstr + " %al");
        emit("  movzbl %al, %eax");
        emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
    }

    std::string X86Generator::generate(const IRList &ir)
    {
        code_.clear();
        varOffsets_.clear();
        stackSize_ = 0;
        nextOffset_ = 0;

        allocateVars(ir);
        int frameSize = alignedFrameSize();

        emit(".text");
        emit(".global " + symbol("main"));
        emit(symbol("main") + ":");
        emit("  pushq %rbp");
        emit("  movq %rsp, %rbp");
        if (frameSize > 0)
            emit("  subq $" + std::to_string(frameSize) + ", %rsp");

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
                if (isIntegerText(q.arg1))
                {
                    if (std::stoi(q.arg1) == 0)
                        emit("  jmp " + q.result);
                }
                else
                {
                    emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), %rax");
                    emit("  testq %rax, %rax");
                    emit("  jz " + q.result);
                }
                break;

            case IROp::ASSIGN:
                loadValueToRax(q.arg1);
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::READ:
                emit("  leaq .LC1(%rip), " + firstArgReg());
                emit("  leaq " + std::to_string(getVarOffset(q.result)) + "(%rbp), " + secondArgReg());
                emit("  movq $0, %rax");
                emit("  callq " + symbol("scanf"));
                emit("  movl " + std::to_string(getVarOffset(q.result)) + "(%rbp), %eax");
                emit("  movslq %eax, %rax");
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::WRITE:
                if (isIntegerText(q.arg1))
                    emit("  movq $" + q.arg1 + ", " + firstArgReg());
                else
                    emit("  movq " + std::to_string(getVarOffset(q.arg1)) + "(%rbp), " + firstArgReg());
                emit("  callq " + symbol("print_int"));
                break;

            case IROp::RETURN:
                loadValueToRax(q.arg1);
                emit("  movq %rbp, %rsp");
                emit("  popq %rbp");
                emit("  retq");
                break;

            case IROp::ADD:
                loadValueToRax(q.arg1);
                if (isIntegerText(q.arg2))
                    emit("  addq $" + q.arg2 + ", %rax");
                else
                    emit("  addq " + std::to_string(getVarOffset(q.arg2)) + "(%rbp), %rax");
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::SUB:
                loadValueToRax(q.arg1);
                if (isIntegerText(q.arg2))
                    emit("  subq $" + q.arg2 + ", %rax");
                else
                    emit("  subq " + std::to_string(getVarOffset(q.arg2)) + "(%rbp), %rax");
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::MUL:
                loadValueToRax(q.arg1);
                if (isIntegerText(q.arg2))
                    emit("  imulq $" + q.arg2 + ", %rax");
                else
                    emit("  imulq " + std::to_string(getVarOffset(q.arg2)) + "(%rbp), %rax");
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::DIV:
                loadValueToRax(q.arg1);
                emit("  cqto");
                if (isIntegerText(q.arg2))
                {
                    emit("  movq $" + q.arg2 + ", %r10");
                    emit("  idivq %r10");
                }
                else
                {
                    emit("  idivq " + std::to_string(getVarOffset(q.arg2)) + "(%rbp)");
                }
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::MOD:
                loadValueToRax(q.arg1);
                emit("  cqto");
                if (isIntegerText(q.arg2))
                {
                    emit("  movq $" + q.arg2 + ", %r10");
                    emit("  idivq %r10");
                }
                else
                {
                    emit("  idivq " + std::to_string(getVarOffset(q.arg2)) + "(%rbp)");
                }
                emit("  movq %rdx, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::LT:
                emitCompare(q, "setl");
                break;
            case IROp::LE:
                emitCompare(q, "setle");
                break;
            case IROp::GT:
                emitCompare(q, "setg");
                break;
            case IROp::GE:
                emitCompare(q, "setge");
                break;
            case IROp::EQ:
                emitCompare(q, "sete");
                break;
            case IROp::NE:
                emitCompare(q, "setne");
                break;

            case IROp::AND:
                loadValueToRax(q.arg1);
                emit("  testq %rax, %rax");
                emit("  setne %al");
                emit("  movzbl %al, %eax");
                if (isIntegerText(q.arg2))
                {
                    emit(std::stoi(q.arg2) == 0 ? "  andq $0, %rax" : "  andq $1, %rax");
                }
                else
                {
                    emit("  movq " + std::to_string(getVarOffset(q.arg2)) + "(%rbp), %r10");
                    emit("  testq %r10, %r10");
                    emit("  setne %r10b");
                    emit("  movzbl %r10b, %r10d");
                    emit("  andq %r10, %rax");
                }
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::OR:
                loadValueToRax(q.arg1);
                emit("  testq %rax, %rax");
                emit("  setne %al");
                emit("  movzbl %al, %eax");
                if (isIntegerText(q.arg2))
                {
                    emit(std::stoi(q.arg2) == 0 ? "  orq $0, %rax" : "  orq $1, %rax");
                }
                else
                {
                    emit("  movq " + std::to_string(getVarOffset(q.arg2)) + "(%rbp), %r10");
                    emit("  testq %r10, %r10");
                    emit("  setne %r10b");
                    emit("  movzbl %r10b, %r10d");
                    emit("  orq %r10, %rax");
                }
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::NOT:
                loadValueToRax(q.arg1);
                emit("  testq %rax, %rax");
                emit("  sete %al");
                emit("  movzbl %al, %eax");
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            case IROp::NEG:
                loadValueToRax(q.arg1);
                emit("  negq %rax");
                emit("  movq %rax, " + std::to_string(getVarOffset(q.result)) + "(%rbp)");
                break;

            default:
                break;
            }
        }

        emit(".data");
        emit(".LC0: .string \"%d\\n\"");
        emit(".LC1: .string \"%d\"");

        emit(".text");
        emit(symbol("print_int") + ":");
        emit("  pushq %rbp");
        emit("  movq %rsp, %rbp");
        if (kWindowsAbi)
        {
            emit("  subq $32, %rsp");
            emit("  movq %rcx, %rdx");
            emit("  leaq .LC0(%rip), %rcx");
        }
        else
        {
            emit("  movq %rdi, %rsi");
            emit("  leaq .LC0(%rip), %rdi");
        }
        emit("  movq $0, %rax");
        emit("  callq " + symbol("printf"));
        emit("  movq %rbp, %rsp");
        emit("  popq %rbp");
        emit("  retq");

        std::string result;
        for (const auto &line : code_)
            result += line + "\n";
        return result;
    }

} // namespace minic
