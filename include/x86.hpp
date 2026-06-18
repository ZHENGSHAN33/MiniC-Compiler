#pragma once

#include <string>
#include <vector>
#include <map>
#include "ir.hpp"

namespace minic
{

    class X86Generator
    {
    public:
        std::string generate(const IRList &ir);

    private:
        void emit(const std::string &line);
        void emitln(const std::string &line);
        int getVarOffset(const std::string &name);
        void allocateVars(const IRList &ir);
        bool isIntegerText(const std::string &s);

        std::vector<std::string> code_;
        std::map<std::string, int> varOffsets_;
        int stackSize_ = 0;
        int nextOffset_ = 0;
    };

} // namespace minic