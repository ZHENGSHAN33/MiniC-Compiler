# Mini-C Compiler

这是一个面向编译原理大作业的 Mini-C 教学编译器。项目重点不是把 C 语言做全，而是把一个小语言从源程序一路走到可运行结果：词法分析、语法分析、语义检查、中间代码、简单优化、VM 后端和解释执行。

## 已支持功能

- 类型：`int`、`bool`
- 语句：变量声明、赋值、`if/else`、`while`、`break`、`continue`、`return`
- 输入输出：`read(x);`、`write(expr);`
- 表达式：算术、关系、相等、逻辑和一元运算
- 展示输出：Token、AST、AST DOT、符号表、IR、优化后 IR、VM 指令、类 x86 汇编
- 错误处理：词法错误、语法错误、语义错误，全部带行号和列号

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

## 小组协作方式

这个仓库先作为可运行基线版本。组员不要直接在 `main` 分支上改，建议每个人按自己的模块建分支编写代码之后提交pr：

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

确认能通过后再提交：

```powershell
git add .
git commit -m "feat(模块名): 简短说明"
git push -u origin feature/自己的模块名
```

然后在 GitHub 上发 Pull Request，由组长检查后合并。

目前 `src/main.cpp` 是一个完整集成版，方便先跑通演示。后续根据分工成员要拆模块，可以按下面方向逐步拆，不要一次性大改：

```text
lexer      -> 词法分析相关代码
parser     -> 语法分析和 AST 相关代码
semantic   -> 符号表和类型检查
ir         -> 中间代码生成与优化
backend    -> VM 指令和 x86 风格汇编输出
tests      -> 正例、反例和自动测试
```


## 目录说明

```text
include/common.hpp          公共数据结构和接口
src/main.cpp                当前集成版编译器实现
tests/                      正例和反例
scripts/run_all_tests.ps1   自动测试脚本
```
