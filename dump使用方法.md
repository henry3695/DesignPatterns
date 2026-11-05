**🛠️ 全平台 Release 程序崩溃/卡死 Dump 排查实战指南**

本指南记录了在 **Windows** 和 **Linux** 环境下，如何让 Release 程序崩溃/卡死时生成 Dump 文件，并利用 **WinDbg** 和 **GDB** 精准定位到出错源码行号的完整流程。

---

📥 实验准备：崩溃示例代码

编写一个简单的空指针崩溃程序 `crash_demo.cpp`。

cpp

```
#include <iostream>
// 💥 触发空指针崩溃的函数
void bad_function() 
{    
    std::cout << "准备触发空指针崩溃..." << std::endl;    
    int* ptr = nullptr;     
    *ptr = 42; // 第 7 行：向空地址写数据
 }
 
 // 中间调用函数
 void middle_function() 
 {    
     bad_function();
 }
 
 int main() 
 {    
     std::cout << "程序启动..." << std::endl;    
     middle_function();    
     return 0;
 }
```

请谨慎使用此类代码。

---

🪟 Windows 篇：Release 崩溃排查 (WinDbg)

1. 编译 Release 程序（保留 PDB 调试符号）

打开 Visual Studio 的 **Developer Command Prompt**，执行以下命令：

bash

```
cl.exe /O2 /Zi /EHsc crash_demo.cpp
```

请谨慎使用此类代码。

- `/O2`：开启 Release 速度优化。
- `/Zi`：生成 `crash_demo.pdb` 符号文件（**核心：排查必留**）。
2. 配置系统自动捕获 Dump (WER 机制)

用**管理员权限**打开 CMD/PowerShell，执行以下命令（让系统在程序崩溃时自动在 `C:\CrashDumps` 生成完整内存转储）：

cmd

```
# 创建存储目录
mkdir C:\CrashDumps

# 注册 WER 捕获项
reg add "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\crash_demo.exe" /v DumpFolder /t REG_EXPAND_SZ /d "C:\CrashDumps" /f
reg add "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\crash_demo.exe" /v DumpType /t REG_DWORD /d 2 /f

```

请谨慎使用此类代码。

3. 运行并触发崩溃

直接双击或在命令行运行 `crash_demo.exe`，程序闪退。此时前往 `C:\CrashDumps`，会看到已生成类似 `crash_demo.exe.1234.dmp` 的文件。

4. WinDbg 分析定位

1. 打开 **WinDbg Preview**，点击 `File` -> `Open dump file`，加载刚才的 `.dmp` 文件。

2. 在底部的 Command 框中配置符号文件（PDB）路径并加载：

   dbgcmd

   ```
   .sympath+ C:\您的源码和PDB所在目录
   .reload
   ```

   请谨慎使用此类代码。

3. 执行万能分析命令：

   dbgcmd

   ```
   !analyze -v
   ```

   请谨慎使用此类代码。

4. **定位结果**：在输出报告的 `STACK_TEXT` 或 `FAULTING_IP` 区域，可直接看到崩溃位置：

   text

   ```
   crash_demo!bad_function+0x7 [c:\path\to\crash_demo.cpp @ 7]
   crash_demo!middle_function+0x9
   crash_demo!main+0x9
   ```

   请谨慎使用此类代码。

   *结论：程序精准死在 `crash_demo.cpp` 的第 7 行。*

---

🐧 Linux 篇：Release 崩溃排查 (GDB)

1. 编译 Release 程序（保留调试符号）

在 Linux 终端执行以下命令：

bash

```
g++ -O2 -g crash_demo.cpp -o crash_demo
```

请谨慎使用此类代码。

- `-O2`：开启 Release 级别优化。
- `-g`：在可执行文件中保留调试符号表。
2. 解除系统 Core 限制并配置路径

Linux 默认不允许生成 Core 文件，需手动解除限制：

bash

```
# 允许当前终端及子进程生成无限大的 Core 文件
ulimit -c unlimited

# (可选) 配置内核参数，使 Core 文件按“进程名-PID-时间”规范命名
sudo sysctl -w kernel.core_pattern=core-%e-%p-%t
```

请谨慎使用此类代码。

3. 运行并触发崩溃

bash

```
./crash_demo
```

请谨慎使用此类代码。

程序输出后提示 `Segmentation fault (core dumped)`，当前目录下会生成一个名为 `core-crash_demo-XXXX` 的文件。

4. GDB 分析定位

使用 GDB 同时加载**原可执行程序**与 **Core 文件**：

bash

```
gdb ./crash_demo core-crash_demo-XXXX
```

请谨慎使用此类代码。

GDB 启动后会自动分析，屏幕上会立刻弹出致命伤位置：

text

```
Program terminated with signal SIGSEGV, Segmentation fault.
#0  0x0000555555555167 in bad_function () at crash_demo.cpp:7
7	    *ptr = 42; // 💥 这里会发生崩溃
```

请谨慎使用此类代码。

如果想看完整的调用链，输入 `bt` (Backtrace) 命令：

gdb

```
(gdb) bt
#0  0x0000555555555167 in bad_function () at crash_demo.cpp:7
#1  0x0000555555555189 in middle_function () at crash_demo.cpp:12
#2  0x0000555555555065 in main () at crash_demo.cpp:17
```

请谨慎使用此类代码。

*结论：调用栈清晰完整，错误直指第 7 行。*

---

💡 核心避坑总结与备忘

1. **Release 能不能查？** 能。但编译时 Windows 必须生成 `.pdb`，Linux 必须带 `-g`。
2. **线上发布程序体积过大怎么办？**
   - **Windows**：直接发布 `.exe`，把 `.pdb` 留在开发机，分析 Dump 时拿出来对齐。
   - **Linux**：用 `strip` 命令擦除发布版程序的符号表以减小体积，但研发环境必须妥善保存一份未被 `strip`（带调试信息）的原版程序用于配合 Core 分析。
3. **变量显示 ？** 这是 Release 优化的正常现象（变量被优化进寄存器了）。此时应重点看调用栈（Stack）和代码行号，而非局部变量的值。

这份 Markdown 笔记可以直接复制保存。你在实际跟着步骤操作的过程中，有没有在**注册表配置**、**编译参数**或 **WinDbg/GDB 的输出结果**上遇到任何与预期不符的地方？如果有，随时发出来，我们一起来看！


