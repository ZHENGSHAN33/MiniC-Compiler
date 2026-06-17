# Mini-C Compiler

这是一个面向编译原理大作业的 Mini-C 教学编译器。项目目标不是把 C 语言完整复刻一遍，而是把一个小语言从源程序逐步处理到可运行结果：词法分析、语法分析、语义检查、中间代码、简单优化、VM 后端和解释执行。

当前分支是 `feature/semantic`，主要完成语义分析模块拆分和符号表检查。本分支已经包含前面完成的词法分析和语法分析模块，并在 AST 基础上继续进行类型检查、作用域检查和控制流相关检查。

## 已支持功能

- 类型：`int`、`bool`
- 语句：变量声明、赋值、`if/else`、`while`、`break`、`continue`、`return`
- 输入输出：`read(x);`、`write(expr);`
- 表达式：算术、关系、相等、逻辑和一元运算
- 展示输出：Token、AST、AST DOT、符号表、IR、优化后 IR、VM 指令、x86 风格汇编
- 错误处理：词法错误、语法错误、语义错误，均带行号和列号

## feature/semantic 分支完成内容

本分支完成了语义分析模块化：

- 新增 `include/semantic.hpp`，声明 `Semantic` 类和 `typeName` 接口。
- 新增 `src/semantic.cpp`，实现语义分析和符号表输出。
- 维护作用域栈，支持变量和函数符号登记。
- 检查同一作用域内变量重复声明。
- 检查变量使用前是否已经声明。
- 检查赋值语句左右类型是否一致。
- 检查 `if`、`while` 条件是否为 `bool`。
- 检查 `break`、`continue` 是否位于循环内部。
- 检查 `read` 目前只读取 `int` 变量。
- 检查一元、二元表达式操作数类型和返回值类型。
- `build.ps1`、`Makefile`、`scripts/run_all_tests.ps1` 已接入 `src/semantic.cpp`。
- 补充语义分析专项测试，覆盖未声明变量、重复声明、类型不匹配、循环外跳转、条件类型错误、读入 bool、算术操作数错误和返回值类型错误。

当前测试结果：

```text
Total: 25, Passed: 25, Failed: 0
```

## 构建

Windows PowerShell：

```powershell
.\build.ps1
```

如果环境里有 `make`：

```bash
make
```

## 使用

```bash
compiler tests/valid/factorial.mc --tokens
compiler tests/valid/factorial.mc --ast
compiler tests/valid/factorial.mc --dot
compiler tests/valid/factorial.mc --check
compiler tests/valid/factorial.mc --ir
compiler tests/valid/factorial.mc --ir --opt
compiler tests/valid/factorial.mc --vm
compiler tests/valid/factorial.mc --run
compiler tests/valid/factorial.mc -S
```

查看符号表和语义检查结果：

```powershell
.\compiler.exe tests\valid\factorial.mc --check
```

运行阶乘示例：

```powershell
echo 5 | .\compiler.exe tests\valid\factorial.mc --run
```

预期输出：

```text
120
```

## 测试

运行全部测试：

```powershell
.\scripts\run_all_tests.ps1
```

测试内容包括：

- 正确程序：阶乘、求和、逻辑判断、优化样例、嵌套语句和表达式优先级
- 词法错误：非法字符、未闭合注释、非法数字、单个 `&`、单个 `|`
- 语法错误：缺少分号、缺少右括号、非法语句开头
- 语义错误：未声明变量、重复声明、类型不匹配、循环外 `break/continue`、条件类型错误、读入 bool、算术操作数错误、返回值类型错误

## 小组协作方式

这个仓库先作为可运行基线版本。组员不要直接在 `main` 分支上改，建议每个人按自己的模块建分支：

```text
feature/lexer       词法分析
feature/parser      语法分析与 AST
feature/semantic    语义分析与符号表
feature/ir          中间代码与优化
feature/backend     VM / x86 后端
feature/tests       测试用例与测试脚本
```

组员开始写代码前，先执行：

```powershell
git clone https://github.com/ZHENGSHAN33/MiniC-Compiler.git
cd MiniC-Compiler
git checkout -b feature/自己的模块名
```

修改后先本地测试：

```powershell
.\build.ps1
.\scripts\run_all_tests.ps1
```

确认通过后再提交：

```powershell
git add .
git commit -m "feat(模块名): 简短说明"
git push -u origin feature/自己的模块名
```

然后在 GitHub 上发 Pull Request，由组长检查后合并。

## 后续拆分建议

目前词法分析、语法分析和语义分析已经拆出，`src/main.cpp` 仍保留主控流程以及中间代码、后端等集成实现。后续成员可以按同样方式逐步拆分：

```text
ir         -> include/ir.hpp        + src/ir.cpp
backend    -> include/backend.hpp   + src/backend.cpp
tests      -> 正例、反例和自动测试
```

公共结构仍然放在 `include/common.hpp`，原则上由组长统一修改。

## 目录说明

```text
include/common.hpp          公共数据结构和接口
include/lexer.hpp           词法分析模块接口
include/parser.hpp          语法分析模块接口
include/semantic.hpp        语义分析模块接口
src/main.cpp                当前主控流程和后续集成模块
src/lexer.cpp               词法分析模块实现
src/parser.cpp              语法分析模块实现
src/semantic.cpp            语义分析模块实现
tests/                      正例和反例
scripts/run_all_tests.ps1   自动测试脚本
```
