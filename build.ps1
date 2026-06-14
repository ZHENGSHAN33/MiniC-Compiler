$ErrorActionPreference = "Stop"

<<<<<<< HEAD
g++ -std=c++17 -Wall -Wextra -O2 -Iinclude src/main.cpp -o compiler.exe
=======
g++ -std=c++17 -Wall -Wextra -O2 -Iinclude src/main.cpp src/lexer.cpp -o compiler.exe
>>>>>>> 7cf4835 (feat(lexer):完成词法分析器功能)
Write-Host "build ok: compiler.exe"
