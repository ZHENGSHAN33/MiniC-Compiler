#include "common.hpp"

#include "lexer.hpp"
#include "parser.hpp"
#include "semantic.hpp"
#include "ir.hpp"
#include "x86.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <unordered_map>

using namespace minic;

namespace
{

    std::string readFile(const std::string &path)
    {
        std::ifstream in(path);
        if (!in)
            throw std::runtime_error("cannot open input file: " + path);
        std::ostringstream ss;
        ss << in.rdbuf();
        return ss.str();
    }

    bool isIntegerText(const std::string &s)
    {
        if (s.empty())
            return false;
        size_t i = (s[0] == '-') ? 1 : 0;
        if (i == s.size())
            return false;
        for (; i < s.size(); ++i)
            if (!std::isdigit(static_cast<unsigned char>(s[i])))
                return false;
        return true;
    }

    std::vector<VMInst> generateVM(const IRList &ir)
    {
        std::vector<VMInst> code;
        auto load = [&](const std::string &x)
        { code.push_back({isIntegerText(x) ? "PUSH" : "LOAD", x}); };
        for (const auto &q : ir)
        {
            switch (q.op)
            {
            case IROp::LABEL:
                code.push_back({"LABEL", q.result});
                break;
            case IROp::GOTO:
                code.push_back({"JMP", q.result});
                break;
            case IROp::IF_FALSE:
                load(q.arg1);
                code.push_back({"JZ", q.result});
                break;
            case IROp::ASSIGN:
                load(q.arg1);
                code.push_back({"STORE", q.result});
                break;
            case IROp::READ:
                code.push_back({"READ", q.result});
                break;
            case IROp::WRITE:
                load(q.arg1);
                code.push_back({"PRINT", ""});
                break;
            case IROp::RETURN:
                load(q.arg1);
                code.push_back({"RET", ""});
                break;
            case IROp::NOT:
                load(q.arg1);
                code.push_back({"NOT", ""});
                code.push_back({"STORE", q.result});
                break;
            case IROp::NEG:
                load(q.arg1);
                code.push_back({"NEG", ""});
                code.push_back({"STORE", q.result});
                break;
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

    void printVM(std::ostream &out, const std::vector<VMInst> &code)
    {
        for (const auto &inst : code)
        {
            out << inst.op;
            if (!inst.arg.empty())
                out << " " << inst.arg;
            out << "\n";
        }
    }

    int runVM(const std::vector<VMInst> &code, std::istream &in, std::ostream &out)
    {
        std::unordered_map<std::string, int> vars;
        std::unordered_map<std::string, size_t> labels;
        std::vector<int> stack;
        for (size_t i = 0; i < code.size(); ++i)
            if (code[i].op == "LABEL")
                labels[code[i].arg] = i;

        auto pop = [&]()
        {
            int v = stack.empty() ? 0 : stack.back();
            if (!stack.empty())
                stack.pop_back();
            return v;
        };

        for (size_t pc = 0; pc < code.size(); ++pc)
        {
            const auto &inst = code[pc];
            if (inst.op == "PUSH")
                stack.push_back(std::stoi(inst.arg));
            else if (inst.op == "LOAD")
                stack.push_back(vars[inst.arg]);
            else if (inst.op == "STORE")
                vars[inst.arg] = pop();
            else if (inst.op == "READ")
            {
                int v = 0;
                in >> v;
                vars[inst.arg] = v;
            }
            else if (inst.op == "PRINT")
                out << pop() << "\n";
            else if (inst.op == "+")
            {
                int b = pop(), a = pop();
                stack.push_back(a + b);
            }
            else if (inst.op == "-")
            {
                int b = pop(), a = pop();
                stack.push_back(a - b);
            }
            else if (inst.op == "*")
            {
                int b = pop(), a = pop();
                stack.push_back(a * b);
            }
            else if (inst.op == "/")
            {
                int b = pop(), a = pop();
                stack.push_back(b == 0 ? 0 : a / b);
            }
            else if (inst.op == "%")
            {
                int b = pop(), a = pop();
                stack.push_back(b == 0 ? 0 : a % b);
            }
            else if (inst.op == "<")
            {
                int b = pop(), a = pop();
                stack.push_back(a < b);
            }
            else if (inst.op == "<=")
            {
                int b = pop(), a = pop();
                stack.push_back(a <= b);
            }
            else if (inst.op == ">")
            {
                int b = pop(), a = pop();
                stack.push_back(a > b);
            }
            else if (inst.op == ">=")
            {
                int b = pop(), a = pop();
                stack.push_back(a >= b);
            }
            else if (inst.op == "==")
            {
                int b = pop(), a = pop();
                stack.push_back(a == b);
            }
            else if (inst.op == "!=")
            {
                int b = pop(), a = pop();
                stack.push_back(a != b);
            }
            else if (inst.op == "&&")
            {
                int b = pop(), a = pop();
                stack.push_back(a && b);
            }
            else if (inst.op == "||")
            {
                int b = pop(), a = pop();
                stack.push_back(a || b);
            }
            else if (inst.op == "NOT")
                stack.push_back(!pop());
            else if (inst.op == "NEG")
                stack.push_back(-pop());
            else if (inst.op == "JMP")
                pc = labels[inst.arg];
            else if (inst.op == "JZ")
            {
                if (!pop())
                    pc = labels[inst.arg];
            }
            else if (inst.op == "RET")
                return pop();
            else if (inst.op == "HALT")
                return 0;
        }
        return 0;
    }

    std::string generatePseudoX86(const IRList &ir)
    {
        std::ostringstream out;
        out << "; Mini-C pseudo x86-64 output for presentation\n";
        for (const auto &q : ir)
        {
            if (q.op == IROp::LABEL)
                out << q.result << ":\n";
            else if (q.op == IROp::GOTO)
                out << "  jmp " << q.result << "\n";
            else if (q.op == IROp::IF_FALSE)
                out << "  cmp [" << q.arg1 << "], 0\n  je " << q.result << "\n";
            else if (q.op == IROp::ASSIGN)
                out << "  mov [" << q.result << "], " << q.arg1 << "\n";
            else if (q.op == IROp::READ)
                out << "  call read_int ; -> " << q.result << "\n";
            else if (q.op == IROp::WRITE)
                out << "  call print_int ; " << q.arg1 << "\n";
            else if (q.op == IROp::RETURN)
                out << "  mov rax, " << q.arg1 << "\n  ret\n";
            else
                out << "  ; " << q.result << " = " << q.arg1 << " " << irOpName(q.op) << " " << q.arg2 << "\n";
        }
        return out.str();
    }

    struct Pipeline
    {
        TokenList tokens;
        std::unique_ptr<ASTNode> ast;
        Semantic semantic;
        IRList ir;
        IRList optIr;
    };

    bool printErrors(const std::vector<CompileError> &errors)
    {
        for (const auto &e : errors)
            std::cerr << e.str() << "\n";
        return !errors.empty();
    }

    Pipeline compileFrontend(const std::string &source)
    {
        Pipeline p;
        Lexer lexer(source);
        p.tokens = lexer.scan();
        if (printErrors(lexer.errors()))
            throw std::runtime_error("lexical analysis failed");

        Parser parser(p.tokens);
        p.ast = parser.parseProgram();
        if (printErrors(parser.errors()))
            throw std::runtime_error("syntax analysis failed");

        p.semantic.analyze(p.ast.get());
        if (printErrors(p.semantic.errors()))
            throw std::runtime_error("semantic analysis failed");

        IRGenerator gen;
        p.ir = gen.generate(p.ast.get());
        p.optIr = optimizeIR(p.ir);
        return p;
    }

    void help()
    {
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

int main(int argc, char **argv)
{
    if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")
    {
        help();
        return argc < 2 ? 1 : 0;
    }

    std::string path = argv[1];
    std::set<std::string> options;
    for (int i = 2; i < argc; ++i)
        options.insert(argv[i]);

    try
    {
        std::string source = readFile(path);
        bool onlyTokens = options.count("--tokens") > 0;

        if (onlyTokens)
        {
            Lexer lexer(source);
            TokenList tokens = lexer.scan();
            printTokens(std::cout, tokens);
            return printErrors(lexer.errors()) ? 1 : 0;
        }

        Pipeline p = compileFrontend(source);

        if (options.count("--ast"))
        {
            printAST(std::cout, p.ast.get());
        }
        else if (options.count("--dot"))
        {
            DotPrinter printer;
            std::cout << printer.print(p.ast.get());
        }
        else if (options.count("--check"))
        {
            std::cout << "semantic check passed\n";
            p.semantic.printSymbols(std::cout);
        }
        else if (options.count("--ir"))
        {
            printIR(std::cout, options.count("--opt") ? p.optIr : p.ir);
        }
        else if (options.count("--vm"))
        {
            auto vm = generateVM(options.count("--opt") ? p.optIr : p.ir);
            printVM(std::cout, vm);
        }
        else if (options.count("--run"))
        {
            auto vm = generateVM(p.optIr);
            return runVM(vm, std::cin, std::cout);
        }
        else if (options.count("-S"))
        {
            X86Generator x86gen;
            std::cout << x86gen.generate(p.optIr);
        }
        else
        {
            std::cout << "compile passed. Use --help to see display options.\n";
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << "\n";
        return 1;
    }
    return 0;
}
