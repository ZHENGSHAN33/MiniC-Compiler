#pragma once
#include "common.hpp"
#include <ostream>
#include <string>
namespace minic
{

    // AST转三地址IR生成器
    class IRGenerator
    {
    public:
        IRList generate(ASTNode *root);

    private:
        IRList ir_;
        int tempId_ = 0;
        int labelId_ = 0;
        // 保存循环begin/end标签，用于break/continue
        std::vector<std::pair<std::string, std::string>> loopLabels_;

        std::string newTemp();
        std::string newLabel();
        void emit(IROp op, std::string a = "", std::string b = "", std::string r = "");

        void genBlock(ASTNode *block);
        void genStmt(ASTNode *node);
        std::string genExpr(ASTNode *node);
    };

    // IR常量计算辅助函数
    int calc(IROp op, int a, int b);
    // 字符串判断是否纯数字（支持负数）
    bool isIntegerText(const std::string &s);
    // 运算符字符串转IR操作码
    IROp opFromText(const std::string &op);
    // IR操作码转打印字符串
    std::string irOpName(IROp op);

    // IR常量优化：常量折叠、常量传播、不可达代码消除
    IRList optimizeIR(const IRList &input);

    // 打印三地址IR到输出流
    void printIR(std::ostream &out, const IRList &ir);

} // namespace minic