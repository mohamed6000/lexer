@echo off

set CompilerFlags=-g -Wall -Werror -Wextra

pushd ..\build
gcc %CompilerFlags% ..\code\main.cpp ..\code\lexer.cpp -o lexer.exe 
popd