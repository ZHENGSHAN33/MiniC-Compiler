# Mini-C Compiler

这是一个面向编译原理大作业的 Mini-C 教学编译器。项目目标不是把 C 语言完整复刻一遍，而是把一个小语言从源程序逐步处理到可运行结果：词法分析、语法分析、语义检查、中间代码、简单优化、VM 后端和 x86 汇编输出。

当前版本是最终整合版，已经包含词法分析、语法分析、语义分析、中间代码生成、简单 IR 优化、栈式 VM 解释执行和 x86-64 汇编生成。

## 已支持功能

- 类型：`int`、`bool`
- 语句：变量声明、赋值、`if/else`、`while`、`break`、`continue`、`return`
- 输入输出：`read(x);`、`write(expr);`
- 表达式：算术、关系、相等、逻辑和一元运算
- 展示输出：Token、AST、AST DOT、符号表、IR、优化后 IR、VM 指令、x86-64 汇编
- 错误处理：词法错误、语法错误、语义错误，均带行号和列号

## 最终整合内容

- `include/lexer.hpp`、`src/lexer.cpp`：词法分析。
- `include/parser.hpp`、`src/parser.cpp`：递归下降语法分析和 AST 输出。
- `include/semantic.hpp`、`src/semantic.cpp`：符号表、作用域和类型检查。
- `include/ir.hpp`、`src/ir.cpp`：三地址 IR 生成、常量折叠、常量传播和简单不可达代码删除。
- `include/x86.hpp`、`src/x86.cpp`：x86-64 汇编生成。
- `build.ps1`、`Makefile`、`scripts/run_all_tests.ps1` 已接入全部模块。

`-S` 会根据编译平台生成不同符号和调用约定：

- macOS：`_main`、`_printf`、`_scanf`
- Windows / MinGW：`main`、`printf`、`scanf`

当前测试结果：

```text
Total: 30, Passed: 30, Failed: 0
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

生成 x86 汇编：

```powershell
.\compiler.exe tests\valid\sum.mc -S
```

在 Windows / MinGW 下可进一步编译运行：

```powershell
.\compiler.exe tests\valid\sum.mc -S | Out-File -FilePath test.s -Encoding ASCII
gcc -o test.exe test.s
echo 5 | .\test.exe
```

预期输出：

```text
15
```

## 测试

运行全部测试：

```powershell
.\scripts\run_all_tests.ps1
```

测试内容包括：

- 正确程序：阶乘、求和、逻辑判断、优化样例、嵌套语句和表达式优先级
- IR 测试：表达式临时变量生成、复杂赋值 IR、常量折叠和传播
- x86 测试：汇编输出、立即数比较、生成汇编后用 gcc 编译运行
- 词法错误：非法字符、未闭合注释、非法数字、单个 `&`、单个 `|`
- 语法错误：缺少分号、缺少右括号、非法语句开头
- 语义错误：未声明变量、重复声明、类型不匹配、循环外 `break/continue`、条件类型错误、读入 bool、算术操作数错误、返回值类型错误

## 目录说明

```text
include/common.hpp          公共数据结构和接口
include/lexer.hpp           词法分析模块接口
include/parser.hpp          语法分析模块接口
include/semantic.hpp        语义分析模块接口
include/ir.hpp              中间代码模块接口
include/x86.hpp             x86 后端接口
src/main.cpp                主控流程、VM 解释执行
src/lexer.cpp               词法分析模块实现
src/parser.cpp              语法分析模块实现
src/semantic.cpp            语义分析模块实现
src/ir.cpp                  中间代码生成与优化实现
src/x86.cpp                 x86-64 汇编生成实现
tests/                      正例和反例
scripts/run_all_tests.ps1   自动测试脚本
```
