REM This batch file builds for Visual Studio 2019.
cmake -B build -G "Visual Studio 16 2019" -A x64
start build/iglo.sln
