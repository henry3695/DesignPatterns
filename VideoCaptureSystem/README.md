# VideoMonitor —— 边缘端 C++ 视频监控系统

一个单文件（`main.cpp`）实现的智能监控程序：摄像头采集 + 运动检测自动录像 + Web 实时预览/回放，
支持 **Windows / Ubuntu / 树莓派** 三平台，录像为浏览器可直接播放的 **H.264 MP4**（可带音频）。

适合用来学习：OpenCV 视频采集、MOG2 背景建模、ffmpeg 子进程管道编码、跨平台 popen、
Crow HTTP 服务、多线程帧共享、控制台编码与国际化等实战技巧。

---

## 1. 功能特性

- **实时预览**：浏览器打开网页，点「开始视频采集」查看摄像头实时画面（JPEG 串行拉流）
- **运动检测自动录像**：点「开始录制」后布防，画面异动自动开录，静止 5 秒自动收尾，循环待命
- **录像管理**：网页卡片式列表（缩略图 / 大小 / 时长 / 录制时间），点击在线回放
- **音频录制**：自动探测麦克风，录像带 AAC 音轨，音画同步；失败自动降级为纯视频
- **多摄像头**：`-l` 枚举设备，`-c N` 指定用哪个（笔记本内置 + USB 摄像头场景）
- **树莓派优化**：优先 `h264_v4l2m2m` 硬件编码，失败自动回退 libx264 软编
- **双语文案**：编译期宏 / 命令行 / 环境变量三层可切换中英文，英文系统不乱码

---

## 2. 整体架构

```
┌─────────────── 采集线程（单线程主循环）───────────────┐
│                                                      │
│  cv::VideoCapture ──> frame                          │
│       │                                              │
│       ├─ [网页开关: 采集开?] ──是──> global_current_frame（供 /snapshot）
│       │                                              │
│       └─ [网页开关: 录制开?] ──是──> MOG2 运动检测    │
│                │                     │               │
│                │              异动>1.5% ──> 启动 ffmpeg 子进程（stdin 管道）
│                │              静止 5s  ──> pclose 收尾保存
│                │              录制中  ──> 墙钟补帧写入管道
└──────────────────────────────────────────────────────┘
                      │
┌─────────────── Crow Web 线程池 ──────────────────────┐
│  GET /            首页（按钮 + 实时画面 + 录像列表）  │
│  GET /api/status  开关状态 JSON                       │
│  GET /api/capture 实时画面开关                        │
│  GET /api/record  自动录像开关                        │
│  GET /snapshot    当前帧 JPEG                        │
│  GET /api/list    录像列表 JSON                      │
│  GET /videos/x    录像点播 /thumb/x 缩略图            │
└──────────────────────────────────────────────────────┘
```

两个网页开关相互独立：可以只看画面不录像，也可以只开录制做后台值守。

---

## 3. 目录结构

```
VideoCaptureSystem/
├── main.cpp              # 全部业务逻辑（约 1800 行，注释很密）
├── CMakeLists.txt        # 双平台构建脚本
├── third_party/asio/     # Crow 依赖的 asio 头文件（Windows 本地化用）
├── ffmpeg/               # Windows 版 ffmpeg.exe / ffprobe.exe 及其 dll
├── samples/test.mp4      # 无摄像头时的测试视频源
├── build-win/            # Windows 构建输出
│   └── Release/
│       ├── video_monitor.exe
│       ├── opencv_world4110.dll 等运行时 dll
│       └── videos/       # 录像输出目录（运行时创建）
└── videos/ffmpeg.log     # ffmpeg 报错日志（定位编码问题用）
```

> 部署到别的 Windows 机器，需要整包带走：
> `video_monitor.exe` + 3 个 `opencv_*.dll` + `ffmpeg/` 整个目录。

---

## 4. 编译

### 依赖

- CMake ≥ 3.15、C++17 编译器
- OpenCV 4.x（含 videoio 模块）
- ffmpeg（系统 PATH 中，或 Windows 放进 `./ffmpeg/`）
- Crow 1.2（amalgamated `crow_all.h`）+ asio 头文件

### Windows（VS2022）

```bat
cmake -S . -B build-win -G "Visual Studio 17 2022" -A x64
cmake --build build-win --config Release
```

- 若 MSBuild 莫名崩溃，检查是否同时存在大小写不同的代理变量
  （`HTTPS_PROXY` 和 `https_proxy` 同时设置会让它挂掉），清掉再编。
- OpenCV DLL 会被 CMake 自动拷到 exe 目录。

### Ubuntu / 树莓派

```bash
sudo apt install libopencv-dev libasio-dev ffmpeg
cmake -S . -B build && cmake --build build -j4
./build/video_monitor
```

树莓派上注意把当前用户加进 video 组：`sudo usermod -aG video $USER`（重新登录生效）。

---

## 5. 运行与配置

### 命令行参数

| 参数 | 说明 |
|---|---|
| `-l, --list-cameras` | 枚举摄像头（编号 + 名称 + 实际分辨率），逐个试开验证 |
| `-c, --camera <N>` | 指定摄像头编号（Windows 用 DirectShow，编号与枚举一致） |
| `-s, --source <S>` | 直接指定视频源：编号 / 本地文件（循环播放）/ rtsp 地址 |
| `-p, --port <N>` | Web 端口，默认 30000（被占用时换一个） |
| `--lang zh\|en\|auto` | 输出语言，默认取编译期宏 |
| `-v, --verbose` | 打印 Crow 的 HTTP 请求日志（默认静音） |
| `-h, --help` | 帮助 |

### 环境变量

| 变量 | 作用 |
|---|---|
| `VIDEO_MONITOR_SOURCE` | 同 `-s` |
| `VIDEO_MONITOR_PORT` | 同 `-p` |
| `VIDEO_MONITOR_FFMPEG` | 指定 ffmpeg 路径 |
| `VIDEO_MONITOR_LANG` | zh / en / auto |
| `CAM_WIDTH` / `CAM_HEIGHT` | 请求分辨率（默认 640x480） |
| `CAM_FOURCC` / `CAM_FPS` | 强制像素格式（如 MJPG）/ 帧率 |
| `REC_AUDIO=0` | 关闭录音 |
| `AUDIO_DEV=...` | 手动指定音频设备名 |

### 网页使用

启动后浏览器访问 `http://<本机IP>:30000`：

1. **▶ 开始视频采集** —— 查看实时画面；再点一次停止并清空画面
2. **● 开始录制** —— 布防运动检测（有异动自动录像）；再点一次立即保存当前片段
3. 右侧录像列表 5 秒自动刷新，点击卡片在线播放

---

## 6. 核心原理（学习重点）

### 6.1 运动检测：MOG2 背景建模

```cpp
auto pMOG2 = cv::createBackgroundSubtractorMOG2(500, 16, true);
pMOG2->apply(frame, fgMask);          // 前景掩码
cv::threshold(fgMask, fgMask, 200, 255, cv::THRESH_BINARY); // 去阴影灰度
double ratio = cv::countNonZero(fgMask) * 100.0 / (width * height);
bool motion = ratio > 1.5;            // 前景超过画面 1.5% 判定为异动
```

- 点「开始录制」时会先 `pMOG2->clear()` 并**预热 10 帧**（只喂背景不判定），
  否则空背景模型会把第一帧整幅当成前景，一点按钮就误录。
- 检测到异动后 `last_motion_time` 持续刷新，静止超过 5 秒才收尾，防止录像断成一截一截。

### 6.2 录像：ffmpeg 子进程 + stdin 管道

```cpp
std::string cmd = ffmpeg_prefix()
    + " -nostdin -use_wallclock_as_timestamps 1 "
    + "-f rawvideo -pix_fmt bgr24 -s 640x480 -r 15 -i - "   // 视频帧从 stdin 来
    + audio.input_args + " "                                 // 麦克风作第二路输入
    + "-vcodec libx264 -pix_fmt yuv420p -preset ultrafast "
    + "-map 0:v -map 1:a -c:a aac -af aresample=async=1:first_pts=0 -shortest "
    + video_path;
FILE* pipe = cross_popen(cmd.c_str(), pipe_mode);   // 跨平台 popen
```

- OpenCV 帧以 `bgr24` 裸流写入管道，ffmpeg 负责编码为 H.264 MP4，
  浏览器可以直接播放（OpenCV VideoWriter 的 mp4v 浏览器放不了，只做保底）。
- **墙钟补帧**：声明帧率取 `min(标称fps, 15)`，按真实时间给每个时隙喂一帧，
  采集慢就重复当前帧。这样 mp4 时长严格等于真实时间，不会快进/晃动。
- 音画同步用 `aresample=async=1` 补偿声卡时钟与系统墙钟的漂移。
- 写管道失败（`fwrite` 返回值不对）说明 ffmpeg 已死，先怀疑麦克风，自动降级纯视频重试。

### 6.3 多线程帧共享

```cpp
std::mutex frame_mutex;
cv::Mat global_current_frame;          // 采集线程写，Web 线程读
{ std::lock_guard<std::mutex> lk(frame_mutex); frame.copyTo(global_current_frame); }
```

- `cv::Mat` 赋值是浅拷贝（共享数据指针），必须 `copyTo` 深拷贝，
  否则 Web 线程编码 JPEG 时采集线程正在改同一块内存，会出现撕裂画面。
- 「正在录制」的文件名由采集线程登记（`set_recording_file`），
  列表接口据此跳过该文件——录制中 mp4 的 moov atom 还没写出来，无法解码。

### 6.4 网页开关（按需采集）

- 三个全局原子变量：`g_capture_on` / `g_record_on` / `g_is_recording`，
  Web 线程改、采集线程读。
- 采集关：帧不进 `global_current_frame`（并释放旧帧），`/snapshot` 直接 503；
- 录制关：跳过 MOG2 和写盘，只保持摄像头出帧防止缓冲堆积；
- 关录制瞬间：立即 `pclose` 收尾，已写入的内容不丢。
- 前端每 2 秒轮询 `/api/status` 同步按钮状态，多设备/刷新后保持一致。

### 6.5 控制台双语与编码

```
OUTPUT_LANG 宏（main.cpp 顶部，1=中文 0=英文 -1=跟随系统）
  > 环境变量 VIDEO_MONITOR_LANG > 命令行 --lang zh|en|auto
```

- 中文 Windows 的 cmd 默认 GBK(936)，程序输出 UTF-8 会乱码 →
  内置 `ConsoleStreambuf` 做 UTF-8 → 当前代码页的转换；
- 英文系统既没有对应代码页也常没中文字体 → 所有文案 `TR("中文", "English")` 双语，
  英文模式下设备名等非 ASCII 字符替换为 `?`，保证日志纯 ASCII。

---

## 7. 踩坑记录（真实调试经验）

| 问题 | 原因 | 解决 |
|---|---|---|
| `fatal error: asio.hpp` | Crow 依赖 asio 但系统没装 | `apt install libasio-dev`，Windows 放 `third_party/asio` |
| 录像 2.7 倍速快进、画面"晃动" | 声明 30fps 实际只有 ~11fps，帧被压进 30fps 时间轴 | 降声明帧率至 15 + **墙钟补帧** |
| `moov atom not found` | 录制中的 mp4 moov 在文件尾，此时打开必失败 | 采集线程登记录制中文件名，列表/缩略图跳过 |
| `Corrupt JPEG data` 刷屏 | 强制 MJPG 后该摄像头/驱动返回截断帧 | 不强制 FOURCC，用驱动原生格式（YUY2） |
| `buffer write error` 刷屏 | 前端并发拉快照，浏览器取消未完成请求 | 前端改**串行拉图**（上一张完成再发下一张） |
| cmd 中文乱码 | UTF-8 输出 vs GBK 代码页 | `ConsoleStreambuf` 转换层 |
| 英文系统中文变方块 | 代码页 437/1252 + 没中文字体 | 双语文案，英文模式输出纯 ASCII |
| `[错误] 无法启动 ffmpeg` | Windows `_popen` 不认 POSIX mode `"w"` 外的 `"wb"`（反过来 Linux 不认 `"wb"`） | 按平台分别传 `wb`/`w` |
| ffmpeg 报"不是内部或外部命令" | 给带空格/反斜杠的路径加引号的方式不对 | `ffmpeg_prefix()` 统一处理引号 |
| 音频设备打开失败 | 中文设备名经 `_popen` 窄字符被编码损坏 | 用 ffmpeg 列出的 ASCII `Alternative name` |
| MSBuild 启动即崩 | `HTTPS_PROXY` 与 `https_proxy` 同时存在 | 构建前 `env -u` 清掉大小写各一份 |
| 程序起不来、端口没监听 | 30000 被系统服务占用 | `-p` 换端口；`netstat -ano` 排查 |

---

## 8. 常见问题排查

**Q: 启动后网页打不开？**
先看控制台实际监听端口（可能被占换掉了），再 `netstat -ano | findstr 30000` 确认；
本机测试 curl 记得加 `--noproxy '*'`，否则请求会走系统代理。

**Q: 列表里"没有找到任何摄像头"？**
`video_monitor.exe -l` 重新枚举；USB 摄像头刚插入时驱动要装几秒；
设备管理器确认 Camera 类下设备状态是 OK。

**Q: 只录视频没声音？**
看启动日志：麦克风被占用/探测失败会明确提示；`REC_AUDIO=0` 会主动关闭；
编码失败自动降级时会有 `[警告]` 说明，详情看 `videos/ffmpeg.log`。

**Q: 树莓派上录像卡顿？**
确认走的是 `h264_v4l2m2m` 硬编（日志会打印），软编 libx264 在 Pi 上跑不动高帧率；
必要时 `CAM_WIDTH=640 CAM_HEIGHT=480 CAM_FPS=15` 降低采集负担。

---

## 9. 二次开发建议

- 想加rtsp 网络摄像头：`-s rtsp://ip:554/stream1` 已支持，无需改代码
- 想改运动灵敏度：调 `main.cpp` 里 `motion_percentage > 1.5` 的阈值
- 想改录像时长策略：调 `record_extend_seconds`（静止后延长录制秒数）
- 想换成持续录像模式：把「异动才开录」改成「录制开关开着就写帧」即可，
  管道写入逻辑（墙钟补帧部分）完全可复用
