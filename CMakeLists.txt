cmake_minimum_required(VERSION 2.8.12)

project(DesignPatterns)
# Enable C++11
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS   "-g")                     # 调试信息
#set(CMAKE_CXX_FLAGS   "-Wall")                  # 开启所有警告

message(" --------- Files Test A----- \n")

# 添加各个子目录
set(CPP_DIR ./Factory_Method ./Abstract_Factory)

# 遍历各子目录
foreach(SUB_DIR ${CPP_DIR})
    message(STATUS "---Sub Dir: ${SUB_DIR}")

    #file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/${SUB_DIR}/*.cpp")
    #message(STATUS "---all cpp: ${SRC}")
	
	string(SUBSTRING ${SUB_DIR}  2 -1 SUB_DIR)
	message(STATUS "---Sub Dir: ${SUB_DIR}")
	
	file(GLOB ALLHEADERS ${CMAKE_CURRENT_SOURCE_DIR}/${SUB_DIR}/*.h)
	
	message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}/${SUB_DIR}/*.h")
	
	# 查找当前目录下的所有源文件
	# 并将名称保存到 DIR_SRCS 变量
	aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/${SUB_DIR} DIR_SRCS)
	message(STATUS "---all cpp: ${DIR_SRCS}")
	message(STATUS "---all h: ${ALLHEADERS}")

	# 指定生成目标 
	#string(SUBSTRING ${SUB_DIR}  2 -1 exeName)
	add_executable(${SUB_DIR} ${DIR_SRCS} ${ALLHEADERS})
	set(DIR_SRCS "")
	set(ALLHEADERS "")


    #foreach(CPP ${SRC})
    #   message(STATUS "---Create Exe From: ${CPP}")

    #       STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" FILE_NAME ${CPP})
    #       MESSAGE(" --- FILE_NAME:  ${FILE_NAME}")

	#   	string(SUBSTRING ${SUB_DIR}  2 -1 exeName)
    #       add_executable(${exeName} ${CPP})

    #   endforeach(CPP)    

endforeach(SUB_DIR)

message(" --------- Files Test B----- \n")
