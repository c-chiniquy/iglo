@echo off
GOTO:MAIN

:compile
set SHADER_PATH=..\src\shaders\%~1.h
echo Compiling %~4
break>"%SHADER_PATH%"
break>"temp1.h"
break>"temp2.h"

dxc.exe "%~4" -E %~2 -Vn "g_%~1" -Fh "temp1.h" -T %~3 -WX -Qstrip_debug -spirv || goto :error
dxc.exe "%~4" -E %~2 -Vn "g_%~1" -Fh "temp2.h" -T %~3 -WX -Qstrip_debug -Qstrip_reflect || goto :error

echo #ifdef IGLO_VULKAN>>"%SHADER_PATH%"
echo.>> "%SHADER_PATH%"
type "temp1.h">> "%SHADER_PATH%"
echo.>> "%SHADER_PATH%"
echo #else>> "%SHADER_PATH%"
echo.>> "%SHADER_PATH%"
type "temp2.h">> "%SHADER_PATH%"
echo.>> "%SHADER_PATH%"
echo #endif>> "%SHADER_PATH%"
:cleanup
if exist "temp1.h" (del "temp1.h")
if exist "temp2.h" (del "temp2.h")
EXIT /B 0

:error
if exist ".\%SHADER_PATH%" (del ".\%SHADER_PATH%")
goto :cleanup

:MAIN

echo All shaders will be compiled into header files using DirectXShaderCompiler.
echo The header files will be placed in the /src/shaders/ folder and are compatible with both D3D12 and Vulkan (SPIR-V).
echo You can download DirectXShaderCompiler here: https://github.com/microsoft/DirectXShaderCompiler
echo Make sure "dxc.exe" is added as an environment variable before running this batch file.
echo:
pause
echo:

for /f "delims=|" %%f in ('dir /b /a-d .\ps\') do (
call :compile %%~nf PSMain ps_6_6 ".\ps\%%f"
)
for /f "delims=|" %%f in ('dir /b /a-d .\vs\') do (
call :compile %%~nf VSMain vs_6_6 ".\vs\%%f"
)
for /f "delims=|" %%f in ('dir /b /a-d .\cs\') do (
call :compile %%~nf CSMain cs_6_6 ".\cs\%%f"
)

echo:
echo Done! If no error messages were printed, then all shaders compiled with no issues.
echo:
pause
