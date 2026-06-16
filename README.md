# Mini-C Compiler

这是一个面向编译原理大作业的 Mini-C 教学编译器。项目目标不是把 C 语言做全，而是把一个小语言从源程序一路走到可运行结果：词法分析、语法分析、语义检查、中间代码、简单优化、VM 后端和解释执行。

当前分支是 `feature/lexer`，主要工作是把词法分析器从原来的集成版 `main.cpp` 中拆出来，形成独立的 `include/lexer.hpp` 和 `src/lexer.cpp`，并补充词法专项测试。

## 已支持功能

- 类型：`int`、`bool`
- 语句：变量声明、赋值、`if/else`、`while`、`break`、`continue`、`return`
- 输入输出：`read(x);`、`write(expr);`
- 表达式：算术、关系、相等、逻辑和一元运算
- 展示输出：Token、AST、AST DOT、符号表、IR、优化后 IR、VM 指令、x86 风格汇编
- 错误处理：词法错误、语法错误、语义错误，全部带行号和列号

## feature/lexer 分支完成内容

本分支完成了词法分析模块化：

- 新增 `include/lexer.hpp`，声明 `Lexer` 类和 `printTokens` 接口。
- 新增 `src/lexer.cpp`，实现词法分析逻辑。
- 支持关键字、标识符、整数常量、运算符、界符识别。
- 支持 `//` 单行注释和 `/* ... */` 块注释。
- 支持非法字符、非法数字、未闭合注释、单个 `&`、单个 `|` 的错误报告。
- 支持行号、列号记录。
- 处理 Windows/UTF-8 文件开头可能出现的 UTF-8 BOM。
- `build.ps1`、`Makefile`、`scripts/run_all_tests.ps1` 已接入 `src/lexer.cpp`。
- 自动测试从 11 项扩展到 15 项。

当前测试结果：

```text
Total: 15, Passed: 15, Failed: 0
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

- 正确程序：阶乘、求和、逻辑判断、优化样例
- 词法错误：非法字符、未闭合注释、非法数字、单个 `&`、单个 `|`
- 语法错误：缺少分号
- 语义错误：未声明变量、类型不匹配、循环外 `break`

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

目前词法分析已经拆出，`src/main.cpp` 仍保留主控流程和其余模块的集成实现。后续成员可以按同样方式逐步拆分：

```text
parser     -> include/parser.hpp    + src/parser.cpp
semantic   -> include/semantic.hpp  + src/semantic.cpp
ir         -> include/ir.hpp        + src/ir.cpp
backend    -> include/backend.hpp   + src/backend.cpp
tests      -> 正例、反例和自动测试
```

公共结构仍然放在 `include/common.hpp`，原则上由组长统一修改。

## 目录说明

```text
include/common.hpp          公共数据结构和接口
include/lexer.hpp           词法分析模块接口
src/main.cpp                当前主控流程和其余集成模块
src/lexer.cpp               词法分析模块实现
tests/                      正例和反例
scripts/run_all_tests.ps1   自动测试脚本
```
