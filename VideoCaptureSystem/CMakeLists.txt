cmake_minimum_required(VERSION 3.16)
project(VideoCaptureSystem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(Threads REQUIRED)

# ---------------- OpenCV ----------------
# 可以用 -DOpenCV_DIR=... 指定自己的安装位置；Windows 上未指定时自动找常见路径
if(WIN32 AND NOT OpenCV_DIR)
    foreach(_cand "C:/opencv/opencv/build" "C:/opencv/build" "C:/OpenCV/opencv/build")
        if(EXISTS "${_cand}/OpenCVConfig.cmake")
            set(OpenCV_DIR "${_cand}" CACHE PATH "OpenCV build directory" FORCE)
            break()
        endif()
    endforeach()
endif()

# video / videoio / imgcodecs 分别提供 MOG2、摄像头、JPEG 编解码
find_package(OpenCV REQUIRED COMPONENTS core imgproc video videoio imgcodecs)
message(STATUS "OpenCV: ${OpenCV_VERSION} @ ${OpenCV_DIR}")

add_executable(video_monitor main.cpp)

# ---------------- 日志/界面语言 ----------------
# 不传这个变量时，用 main.cpp 最上面那个 #define OUTPUT_LANG（默认 1 = 中文），
# 这样直接改源码里的数字就能切换，不用动 CMake。
# 也可以在这里覆盖（优先级高于源码）：cmake -DOUTPUT_LANG=en
set(OUTPUT_LANG "" CACHE STRING "Console/UI language: zh, en, auto (empty = use main.cpp default)")
set_property(CACHE OUTPUT_LANG PROPERTY STRINGS "" zh en auto)

if(OUTPUT_LANG STREQUAL "")
    message(STATUS "输出语言: 使用 main.cpp 里 OUTPUT_LANG 的默认值")
elseif(OUTPUT_LANG STREQUAL "zh")
    target_compile_definitions(video_monitor PRIVATE OUTPUT_LANG=1)
    message(STATUS "输出语言: 中文 (OUTPUT_LANG=1)")
elseif(OUTPUT_LANG STREQUAL "en")
    target_compile_definitions(video_monitor PRIVATE OUTPUT_LANG=0)
    message(STATUS "输出语言: 英文 (OUTPUT_LANG=0)")
elseif(OUTPUT_LANG STREQUAL "auto")
    target_compile_definitions(video_monitor PRIVATE OUTPUT_LANG=-1)
    message(STATUS "输出语言: 跟随系统 (OUTPUT_LANG=-1)")
else()
    message(FATAL_ERROR "OUTPUT_LANG 只能是 zh / en / auto，当前是 '${OUTPUT_LANG}'")
endif()

target_include_directories(video_monitor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(video_monitor PRIVATE
    ${OpenCV_LIBS}
    Threads::Threads
)

# ---------------- Crow 的网络依赖 Asio ----------------
# crow_all.h 通过 #include <asio.hpp> 使用「独立版 Asio」，默认不会用 Boost。
#   1) 项目内置 third_party/asio/include（推荐，离线可用，Windows 就是用它）
#   2) 系统安装：Ubuntu/Pi 上 sudo apt install libasio-dev
#   3) 或改用 Boost.Asio：sudo apt install libboost-system-dev
#      cmake -DVIDEO_MONITOR_USE_BOOST_ASIO=ON ..
option(VIDEO_MONITOR_USE_BOOST_ASIO "Use Boost.Asio instead of standalone Asio" OFF)

if(VIDEO_MONITOR_USE_BOOST_ASIO)
    find_package(Boost 1.70 REQUIRED COMPONENTS system)
    target_compile_definitions(video_monitor PRIVATE CROW_USE_BOOST)
    target_link_libraries(video_monitor PRIVATE Boost::system)
    message(STATUS "Crow 使用 Boost.Asio: ${Boost_INCLUDE_DIRS}")
else()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/include/asio.hpp")
        set(ASIO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/asio/include")
    else()
        find_path(ASIO_INCLUDE_DIR NAMES asio.hpp)
    endif()
    if(NOT ASIO_INCLUDE_DIR)
        message(FATAL_ERROR
            "未找到 asio.hpp。请任选其一：\n"
            "  1) 把 asio 头文件放到 third_party/asio/include/\n"
            "  2) sudo apt install libasio-dev\n"
            "  3) sudo apt install libboost-system-dev && cmake -DVIDEO_MONITOR_USE_BOOST_ASIO=ON ..")
    endif()
    target_include_directories(video_monitor PRIVATE ${ASIO_INCLUDE_DIR})
    message(STATUS "Crow 使用独立版 Asio: ${ASIO_INCLUDE_DIR}")
endif()

# ---------------- Windows 专属 ----------------
if(WIN32)
    # Asio 在 Windows 上依赖 Winsock
    target_link_libraries(video_monitor PRIVATE ws2_32 mswsock)

    # 源码里的中文是 UTF-8，MSVC 必须显式指定，否则字符串会变乱码
    if(MSVC)
        target_compile_options(video_monitor PRIVATE /utf-8 /bigobj)
    endif()

    # 把 OpenCV 运行时 DLL 拷到 exe 同目录，命令行直接跑、不用配 PATH
    file(GLOB _ocv_dlls "${OpenCV_DIR}/x64/*/bin/*.dll")
    # 去掉调试版（xxx4110d.dll），Release 用不到，白占 120MB+
    list(FILTER _ocv_dlls EXCLUDE REGEX "d\\.dll$")
    if(_ocv_dlls)
        add_custom_command(TARGET video_monitor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${_ocv_dlls} $<TARGET_FILE_DIR:video_monitor>
            COMMENT "复制 OpenCV 运行时 DLL 到输出目录")
    endif()
endif()
