#pragma once

#include "common.hpp"

#include <string>
#include <vector>
#include <utility>
#include <iostream>

namespace minic
{

    IROp opFromText(const std::string &op);

    std::string irOpName(IROp op);

    class IRGenerator
    {
    public:
        IRList generate(ASTNode *root);

    private:
        IRList ir_;

        int tempId_ = 0;
        int labelId_ = 0;

        std::vector<std::pair<std::string, std::string>> loopLabels_;

        std::string newTemp();
        std::string newLabel();

        void emit(
            IROp op,
            std::string a = "",
            std::string b = "",
            std::string r = "");

        void genBlock(ASTNode *block);
        void genStmt(ASTNode *node);
        std::string genExpr(ASTNode *node);
    };

    IRList optimizeIR(const IRList &input);

    void printIR(std::ostream &out, const IRList &ir);

}