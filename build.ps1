$ErrorActionPreference = "Stop"

g++ -std=c++17 -Wall -Wextra -O2 -Iinclude src/main.cpp -o compiler.exe
Write-Host "build ok: compiler.exe"
