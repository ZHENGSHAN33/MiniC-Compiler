$ErrorActionPreference = "Stop"

& g++ -std=c++17 -Wall -Wextra -O2 -Iinclude src/main.cpp src/lexer.cpp src/parser.cpp src/semantic.cpp src/ir.cpp src/x86.cpp -o compiler.exe
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$ErrorActionPreference = "Continue"

$total = 0
$passed = 0

function Check-Pass($name, $command, $expected) {
    $script:total += 1
    $output = Invoke-Expression $command
    if ($LASTEXITCODE -eq 0 -and ($output -join "`n") -match $expected) {
        Write-Host "[PASS] $name"
        $script:passed += 1
    } else {
        Write-Host "[FAIL] $name"
        Write-Host ($output -join "`n")
    }
}

function Check-Fail($name, $command, $expected) {
    $script:total += 1
    $output = Invoke-Expression "$command 2>&1"
    if ($LASTEXITCODE -ne 0 -and ($output -join "`n") -match $expected) {
        Write-Host "[PASS] $name"
        $script:passed += 1
    } else {
        Write-Host "[FAIL] $name"
        Write-Host ($output -join "`n")
    }
}

Check-Pass "factorial run" "echo 5 | .\compiler.exe tests\valid\factorial.mc --run" "120"
Check-Pass "sum run" "echo 10 | .\compiler.exe tests\valid\sum.mc --run" "55"
Check-Pass "logic run" "echo 6 | .\compiler.exe tests\valid\logic.mc --run" "1"
Check-Pass "token output" ".\compiler.exe tests\valid\factorial.mc --tokens" "KW_WHILE"
Check-Pass "lexer keywords and comments" ".\compiler.exe tests\valid\lexer_keywords_comments.mc --tokens" "KW_BOOL"
Check-Pass "parser precedence and nested statements" ".\compiler.exe tests\valid\parser_precedence_nested.mc --ast" "BinaryExpr \|\|"
Check-Pass "ir arithmetic generation" ".\compiler.exe tests\valid\ir_arithmetic.mc --ir" "t2 = t1 \+ 4"
Check-Pass "ir temporary generation" ".\compiler.exe tests\valid\ir_temporaries.mc --ir" "z = t2"
Check-Pass "optimized ir" ".\compiler.exe tests\valid\optimize.mc --ir --opt" "x = 10"

Check-Pass "x86 assembly output" ".\compiler.exe tests\valid\sum.mc -S" "_main:"
Check-Pass "x86 immediate handling" ".\compiler.exe tests\valid\logic.mc -S" "cmpq \$0"
Check-Pass "x86 sum compile and run" ".\compiler.exe tests\valid\sum.mc -S | Out-File -FilePath test.s -Encoding ASCII; gcc -o test.exe test.s; echo 5 | .\test.exe" "15"

Check-Fail "lexical bad char" ".\compiler.exe tests\invalid_lex\bad_char.mc --tokens" "LexicalError"
Check-Fail "lexical unclosed comment" ".\compiler.exe tests\invalid_lex\unclosed_comment.mc --tokens" "unclosed block comment"
Check-Fail "lexical bad number" ".\compiler.exe tests\invalid_lex\bad_number.mc --tokens" "illegal number format"
Check-Fail "lexical single ampersand" ".\compiler.exe tests\invalid_lex\single_ampersand.mc --tokens" "expected '&' after '&'"
Check-Fail "lexical single pipe" ".\compiler.exe tests\invalid_lex\single_pipe.mc --tokens" "expected '|' after '|'"
Check-Fail "parse missing semicolon" ".\compiler.exe tests\invalid_parse\missing_semicolon.mc --ast" "SyntaxError"
Check-Fail "parse missing right paren" ".\compiler.exe tests\invalid_parse\missing_right_paren.mc --ast" "expected '\)' after expression"
Check-Fail "parse bad statement start" ".\compiler.exe tests\invalid_parse\bad_statement_start.mc --ast" "expected statement"
Check-Fail "semantic undefined variable" ".\compiler.exe tests\invalid_semantic\undefined_var.mc --check" "not declared"
Check-Fail "semantic type mismatch" ".\compiler.exe tests\invalid_semantic\type_mismatch.mc --check" "cannot assign"
Check-Fail "semantic break outside loop" ".\compiler.exe tests\invalid_semantic\break_outside_loop.mc --check" "break must be inside"
Check-Fail "semantic redeclared variable" ".\compiler.exe tests\invalid_semantic\redeclared_var.mc --check" "already declared"
Check-Fail "semantic continue outside loop" ".\compiler.exe tests\invalid_semantic\continue_outside_loop.mc --check" "continue must be inside"
Check-Fail "semantic if condition type" ".\compiler.exe tests\invalid_semantic\if_condition_int.mc --check" "if condition must be bool"
Check-Fail "semantic while condition type" ".\compiler.exe tests\invalid_semantic\while_condition_int.mc --check" "while condition must be bool"
Check-Fail "semantic read bool" ".\compiler.exe tests\invalid_semantic\read_bool.mc --check" "read currently supports int"
Check-Fail "semantic arithmetic operand type" ".\compiler.exe tests\invalid_semantic\arithmetic_bool.mc --check" "expects int operands"
Check-Fail "semantic return type" ".\compiler.exe tests\invalid_semantic\return_type_bool.mc --check" "return type should be int"

Write-Host "Total: $total, Passed: $passed, Failed: $($total - $passed)"
if ($passed -ne $total) { exit 1 }
