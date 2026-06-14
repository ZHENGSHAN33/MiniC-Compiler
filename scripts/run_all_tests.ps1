$ErrorActionPreference = "Stop"

<<<<<<< HEAD
& g++ -std=c++17 -Wall -Wextra -O2 -Iinclude src/main.cpp -o compiler.exe
=======
& g++ -std=c++17 -Wall -Wextra -O2 -Iinclude src/main.cpp src/lexer.cpp -o compiler.exe
>>>>>>> 7cf4835 (feat(lexer):完成词法分析器功能)
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
Check-Pass "optimized ir" ".\compiler.exe tests\valid\optimize.mc --ir --opt" "x = 10"

Check-Fail "lexical bad char" ".\compiler.exe tests\invalid_lex\bad_char.mc --tokens" "LexicalError"
Check-Fail "lexical unclosed comment" ".\compiler.exe tests\invalid_lex\unclosed_comment.mc --tokens" "unclosed block comment"
Check-Fail "parse missing semicolon" ".\compiler.exe tests\invalid_parse\missing_semicolon.mc --ast" "SyntaxError"
Check-Fail "semantic undefined variable" ".\compiler.exe tests\invalid_semantic\undefined_var.mc --check" "not declared"
Check-Fail "semantic type mismatch" ".\compiler.exe tests\invalid_semantic\type_mismatch.mc --check" "cannot assign"
Check-Fail "semantic break outside loop" ".\compiler.exe tests\invalid_semantic\break_outside_loop.mc --check" "break must be inside"

Write-Host "Total: $total, Passed: $passed, Failed: $($total - $passed)"
if ($passed -ne $total) { exit 1 }
