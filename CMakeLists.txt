MESSAGE("")
MESSAGE("#############CMakeing###############")
cmake_minimum_required(VERSION 2.8)

project(WebServer CXX)

set(CXX_FLAGS
    -g
    -Wall
    -std=c++11
    -Wno-unused-parameter
)

MESSAGE("")
MESSAGE("             Platform             ")
MESSAGE(STATUS "operation system is ${CMAKE_SYSTEM}")
IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    MESSAGE(STATUS "current platform: Linux ")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    MESSAGE(STATUS "current platform: MACOS")
    set(CXX_FLAGS ${CXX_FLAGS} -Qunused-arguments)
ELSE ()
    MESSAGE(STATUS "other platform: ${CMAKE_SYSTEM_NAME}")
ENDIF ()
MESSAGE("")

IF(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
ENDIF()


set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O0")
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

##增加macos下cmake与clang的目录查找，在macos上如果用cmake的话/usr/local/bin 和/usr/local/include/目录居然需要手动设置，用gcc就不用，很诡异，linux就下一点问题都没有

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/local/include/ -L/usr/local/lib/")


string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
add_definitions(-D${BUILD_TYPE}_VERSION)


MESSAGE(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})


add_subdirectory(Base)
add_subdirectory(Raven)
add_subdirectory(Host)
add_subdirectory(Client)
#add_subdirectory(FileTransfer)
IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
#add_subdirectory(Log)
add_subdirectory(Server)
ENDIF()



