cmake_minimum_required(VERSION 3.12)
project(OkxBtcQuant CXX)

# 界面语言开关：-DUI_ENGLISH=ON 编译英文版（海外 VPS / 英文系统）
option(UI_ENGLISH "Build with English UI (for overseas VPS / English systems)" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)

add_executable(okxquant
    src/Json.cpp
    src/Util.cpp
    src/Net.cpp
    src/OkxApi.cpp
    src/Strategy.cpp
    src/Risk.cpp
    src/Config.cpp
    src/main.cpp
)

target_include_directories(okxquant PRIVATE include)

if(UI_ENGLISH)
    target_compile_definitions(okxquant PRIVATE UI_ENGLISH)
endif()

target_link_libraries(okxquant PRIVATE OpenSSL::SSL OpenSSL::Crypto)

if(WIN32)
    target_link_libraries(okxquant PRIVATE ws2_32)
    # 静态链接 MinGW 运行时（可选，避免缺 dll）
    if(MINGW)
        target_link_options(okxquant PRIVATE -static-libgcc -static-libstdc++)
    endif()
else()
    target_link_libraries(okxquant PRIVATE pthread)
endif()

if(MSVC)
    # /EHsc: 启用 C++ 异常处理（std::string/STL 需要，否则 C4530 且异常展开不正确）
    # /utf-8: 源码与执行字符集为 UTF-8（中文字符串）
    # _CRT_SECURE_NO_WARNINGS: 屏蔽 fopen 等 C 函数的 C4996 提示
    target_compile_options(okxquant PRIVATE /W3 /utf-8 /EHsc)
    target_compile_definitions(okxquant PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(okxquant PRIVATE -Wall -Wextra)
endif()

# Windows：构建后把 OpenSSL DLL 复制到可执行文件目录，保证 VS 里直接 F5 运行
if(WIN32)
    foreach(dll libssl-1_1-x64__.dll libcrypto-1_1-x64__.dll zlib1__.dll)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/openssl-vc/bin/${dll}")
            add_custom_command(TARGET okxquant POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/openssl-vc/bin/${dll}"
                    $<TARGET_FILE_DIR:okxquant>)
        endif()
    endforeach()
    # CA 证书包（TLS 校验用，Windows 上 OpenSSL 没有系统证书库）
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ca/cacert.pem")
        add_custom_command(TARGET okxquant POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ca/cacert.pem"
                $<TARGET_FILE_DIR:okxquant>)
    endif()
endif()
