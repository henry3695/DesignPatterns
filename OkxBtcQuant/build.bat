@echo off
rem ============================================
rem  VS2022 一键构建（MSVC x64，无需打开 IDE）
rem  用法: build.bat   输出: build-vc\okxquant.exe
rem ============================================
setlocal
cd /d %~dp0

set "VSDIR=D:\Program Files\Microsoft Visual Studio\2022\Community"
set "MSVC=%VSDIR%\VC\Tools\MSVC\14.44.35207"
set "SDK=C:\Program Files (x86)\Windows Kits\10"
set "SDKVER=10.0.26100.0"

if not exist "%MSVC%\bin\HostX64\x64\cl.exe" (
    echo [!] 未找到 MSVC: %MSVC%
    echo     请按实际安装路径修改本脚本开头的变量
    exit /b 1
)

set "PATH=%MSVC%\bin\HostX64\x64;%PATH%"
set "INCLUDE=%MSVC%\include;%SDK%\Include\%SDKVER%\ucrt;%SDK%\Include\%SDKVER%\shared;%SDK%\Include\%SDKVER%\um;%SDK%\Include\%SDKVER%\winrt;%SDK%\Include\%SDKVER%\cppwinrt"
set "LIB=%MSVC%\lib\x64;%SDK%\Lib\%SDKVER%\ucrt\x64;%SDK%\Lib\%SDKVER%\um\x64"

if not exist build-vc mkdir build-vc

cl /nologo /std:c++17 /utf-8 /EHsc /O2 /W3 ^
   /Iinclude /Ithird_party\openssl-vc\include ^
   src\Json.cpp src\Util.cpp src\Net.cpp src\OkxApi.cpp src\Strategy.cpp src\Risk.cpp src\Config.cpp src\main.cpp ^
   /Fe:build-vc\okxquant.exe /Fo:build-vc\ ^
   /link /LIBPATH:third_party\openssl-vc\lib libssl.lib libcrypto.lib ws2_32.lib

if errorlevel 1 (
    echo 构建失败
    exit /b 1
)

copy /y third_party\openssl-vc\bin\*.dll build-vc\ >nul
if exist third_party\ca\cacert.pem copy /y third_party\ca\cacert.pem build-vc\ >nul
echo.
echo 构建完成: build-vc\okxquant.exe
echo 运行: cd build-vc ^&^& okxquant.exe ..\config.json
