cmake_minimum_required(VERSION 2.8)

# Enable C++11
set(CMAKE_CXX_STANDARD 11)

message(" --------- Files Test A----- \n")

# 添加各个子目录
set(CPP_DIR ./Factory_Method ./Abstract_Factory)

# 遍历各子目录
foreach(SUB_DIR ${CPP_DIR})
    message(STATUS "---Sub Dir: ${SUB_DIR}")

    file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/${SUB_DIR}/*.cpp")
    message(STATUS "---all cpp: ${SRC}")

    foreach(CPP ${SRC})
        message(STATUS "---Create Exe From: ${CPP}")

        STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" FILE_NAME ${CPP})
        MESSAGE(" --- FILE_NAME:  ${FILE_NAME}")

		string(SUBSTRING ${SUB_DIR}  2 -1 exeName)
        add_executable(BOOST_${exeName} ${CPP})

    endforeach(CPP)    

endforeach(SUB_DIR)

message(" --------- Files Test B----- \n")
