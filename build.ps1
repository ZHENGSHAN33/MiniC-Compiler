$ErrorActionPreference = "Stop"

& g++ -std=c++17 -Wall -Wextra -O2 -Iinclude src/main.cpp src/lexer.cpp src/parser.cpp src/semantic.cpp -o compiler.exe
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "build ok: compiler.exe"
