# OkxBtcQuant — C++ BTC 合约量化交易（OKX V5，模拟盘）

用标准 C++17 从零实现的 OKX BTC-USDT 永续合约量化交易程序。
**除 OpenSSL 外零第三方依赖**（不用 Boost，网络层全部手写）。

## 功能

- **跨平台**：Windows (Winsock2) / Linux (BSD sockets)，CMake 或直接 g++ 构建
- **网络层（手写，无 Boost）**
  - TCP 连接（非阻塞 connect + 超时）
  - **SOCKS5 代理**（支持无认证 / 用户名密码认证，域名由代理解析）
  - TLS（OpenSSL，SNI + 证书校验，可通过 `ssl.ca_file` 指定 CA）
  - WebSocket 客户端（RFC6455：握手/掩码/分片/ping-pong/自动应用层心跳）
- **OKX V5 接口**
  - REST：HMAC-SHA256 签名、下单（市价/限价）、撤单、余额、持仓、设置杠杆
  - **模拟盘**：REST 自动加 `x-simulated-trading: 1` 头，私有 WS 走 `wspap.okx.com`
  - WebSocket：公共频道（tickers）+ 私有频道（login → orders/account），断线自动重连
- **策略**：EMA 快慢线交叉（金叉做多、死叉做空/平仓），策略代码与实盘同源，方便以后接回测
- **风控**（独立于策略）：单笔限额、总持仓上限、权益回撤熔断、手动 halt/resume
- **控制台命令**：status / pos / bal / open / close / halt / resume / quit

## 构建

OpenSSL 的 MSVC 版（头文件 + 导入库 + DLL）已放在 `third_party/openssl-vc/`，开箱即用，无需再安装。

### 方式一：VS2022 脚本（推荐，秒级构建）
```bat
build.bat
:: 输出 build-vc\okxquant.exe（含所需 DLL）
:: 运行: cd build-vc && okxquant.exe ..\config.json
```
> 脚本开头写死了 VS/SDK 路径（本机实际路径），换机器时改一下前几行变量即可。

### 方式二：VS2022 打开（CMake 集成）
- 已配置 `CMakePresets.json`（预设名 `vs2022`）
- VS2022 打开项目文件夹 → 选择预设 `vs2022` → 生成即可（会自动生成 `build\OkxBtcQuant.sln`）
- 或命令行：`cmake --preset vs2022 && cmake --build build --config Release`

### 方式三：MinGW g++（之前验证过，备用）
```bat
g++ -std=c++17 -O2 -Iinclude -IC:\Strawberry\c\include src\*.cpp -o okxquant.exe ^
    -LC:\Strawberry\c\lib -lssl -lcrypto -lws2_32 -static-libgcc -static-libstdc++
```

### Linux
```sh
sudo apt install g++ cmake libssl-dev
./build.sh
```

### 运行
```sh
./okxquant config.json        # Windows: okxquant.exe
```

## 配置说明（config.json）

| 配置 | 说明 |
|------|------|
| `exchange.demo` | `true`=模拟盘（强烈建议先跑模拟盘） |
| `exchange.api_key/secret_key/passphrase` | **模拟盘 API Key 需在 OKX 网页"模拟交易"入口单独创建**，与实盘 Key 不通用 |
| `proxy.enabled` | `false` 直连；`true` 走 SOCKS5（默认示例 192.168.0.170:9200） |
| `proxy.username/password` | 代理需要认证时填写，否则留空 |
| `ssl.ca_file` | 证书校验失败时可指向 `cacert.pem`（https://curl.se/ca/cacert.pem 下载） |
| `ssl.verify` | 生产环境务必 `true`；内网测试可临时 `false` |
| `strategy.trade_enabled` | **`false` 时只打印信号不下单（dry-run），首次运行保持 false** |
| `strategy.fast_ticks / slow_ticks` | EMA 等效周期（按 tick 数，约等于秒数） |
| `strategy.taker_fee_rate` | taker 手续费率（OKX 合约 0.05%），用于手续费预估与盈亏平衡提示 |
| `strategy.min_edge_bps` | 交叉信号最小优势（基点）。往返一趟 taker 费约 `2×费率×10000` bp（0.05% 时为 10bp），**建议 ≥ 1.5 倍即 15bp 以上**，否则手续费会把利润磨光 |
| `strategy.confirm_ticks` | 候选交叉需连续 N 笔报价方向一致才确认，防噪声假交叉 |
| `strategy.cooldown_sec` | 两次下单最小间隔秒数（手动 open/close 命令不受限） |
| `risk.*` | 单笔限额 / 总仓位上限 / 回撤熔断百分比 |

**手续费与防磨损机制**（v1.2）：策略层在交叉确认时要求快慢线差值超过 `min_edge_bps`，
且需连续 `confirm_ticks` 笔确认；执行层有 `cooldown_sec` 冷却。下单与 status 命令会显示
手续费预估与累计值——**1 张 BTC-USDT-SWAP（0.01 BTC）往返一趟 taker 费约 0.7~1.5 USDT**，
行情没走出这个幅度就别频繁开平。

> 未配置 API Key 时程序以**只读模式**运行：订阅行情、计算并打印信号，不会下单。

## 界面语言（中英双语）

程序所有日志/提示/菜单都支持中英双语，通过编译宏 `UI_ENGLISH` 切换：

| 方式 | 命令 |
|------|------|
| CMake 预设（推荐） | `cmake --preset vs2022-en && cmake --build build-en --config Release` → `build-en\Release\okxquant.exe` |
| CMake 手动 | `cmake -B build-en -DUI_ENGLISH=ON ...` |
| cl 命令行 | 加 `/DUI_ENGLISH` |
| g++ (Linux VPS) | `g++ ... -DUI_ENGLISH ...` |

不定义该宏即为中文版。**海外 VPS / 英文系统建议直接编英文版**，
源码无中文字符串输出，可避免一切代码页/字体问题。
本机已有两个预设：`vs2022`（中文）与 `vs2022-en`（英文）。

## 首次运行建议

1. `trade_enabled: false` 先跑观察信号与连接稳定性
2. 到 OKX 模拟交易页面创建 API Key，填入 config.json
3. 小参数试单（`order_size: 1`，BTC-USDT-SWAP 1 张 = 0.01 BTC）
4. 观察 `okxquant.log` 中的下单/成交/持仓同步日志
5. 一切正常后再考虑实盘（`demo: false`，风险自负）

## 代码结构

```
include/  Json.h   极简 JSON 解析/序列化
          Util.h   日志 / HMAC-SHA256 / 时间 / 字符串
          Net.h    TCP + SOCKS5 + TLS + WebSocket（无 Boost）
          OkxApi.h REST 签名请求 + WS 频道（含模拟盘适配）
          Strategy.h  EMA 交叉策略
          Risk.h   风控（限额/回撤熔断）
          Config.h config.json 加载
src/      对应实现 + main.cpp 编排（事件驱动 + 控制台）
```

## 已知边界 / 扩展方向

- 策略信号基于 tickers 最新价逐 tick 更新 EMA；接 K 线频道可做更标准的指标
- 持仓以 REST 快照 + 订单事件触发刷新同步，高频场景可改为全事件驱动
- 未实现：限价单网格策略、部分成交拆单、多合约、回测回放（接口已按事件驱动设计，易扩展）

## 常见问题

**Q: 提示 `REST 自检失败` / `TLS handshake failed`？**
按顺序排查：
1. **代理**：`proxy.enabled: true` 时确认 host/port 正确（本项目已修好 SOCKS5 端口字节序 bug，实测可通）
2. **证书**：Windows 上 OpenSSL 没有系统证书库，程序会自动在 exe 目录、`third_party/ca/` 找 `cacert.pem`；找不到就报 `certificate verify failed`。把 `third_party/ca/cacert.pem` 复制到 exe 旁边即可，或临时 `ssl.verify: false`
3. **Key 环境**：模拟盘必须在 OKX「模拟交易」页面单独创建 API Key，实盘 Key 在模拟环境会鉴权失败

**Q: 提示 `OK-ACCESS-KEY can not be empty`（50103）？**
本项目已修正：OKX 的鉴权头前缀是 `OK-ACCESS-`（不是 `OKX-ACCESS-`）。若你改过代码注意别写错。

**Q: 控制台窗口很小 / 中文乱码？**
程序启动自动处理：代码页切 UTF-8、窗口放大到 120×35、字体换成"新宋体"18px（点阵字体渲染不了中文，这才是乱码真因）、开启 ANSI 颜色。
启动首行会打印 `控制台: CP=65001 字体=NSimSun 0x16`，可据此确认。
若仍异常，用 **`run.bat` 启动**（额外执行 `chcp 65001`），或改用 Windows Terminal。

**Q: 双击 exe 闪退 / 提示找不到 config.json？**
双击时工作目录是 exe 所在目录，程序会自动向上查找 `config.json`；最省事是双击项目根目录的 `run.bat`（带 pause）。

## 已修复的关键缺陷（记录备查）

| 问题 | 原因 | 修复 |
|------|------|------|
| 代理下所有请求超时 | SOCKS5 CONNECT 的端口字节序双重反转，实际连到 20480 | 按大端直接写高低字节 |
| 鉴权 50103 | 请求头写成 `OKX-ACCESS-*` | 改为 `OK-ACCESS-*` |
| 退出时段错误 | 主线程 `SSL_free` 与读线程并发 | 改为 shutdown 唤醒 + 读线程自行关闭 |
| Windows TLS 校验失败 | 无系统证书库 | 自动加载 `cacert.pem` |

## 免责声明

本程序仅供学习研究。合约交易杠杆风险极高，模拟盘测试充分前切勿实盘。
请遵守所在地区法律法规。
