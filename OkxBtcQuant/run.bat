@echo off
rem ============================================
rem  启动器（双击运行用这个，不要直接双击 exe）
rem  作用: 切 UTF-8 代码页、设窗口标题、退出后停住看结果
rem ============================================
chcp 65001 >nul
title OKX BTC Quant
cd /d %~dp0

if not exist build-vc\okxquant.exe (
    echo [!] 未找到 build-vc\okxquant.exe，请先运行 build.bat 编译
    pause
    exit /b 1
)

build-vc\okxquant.exe config.json

echo.
echo ============================================
echo  程序已退出，按任意键关闭窗口
echo ============================================
pause >nul
