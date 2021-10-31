cmake_minimum_required(VERSION 3.20)
project(luaseri)

set(CMAKE_CXX_STANDARD 11)

add_library(luaseri SHARED lua-seri.cpp)

target_include_directories(luaseri PUBLIC /usr/include/lua5.4)

SET_TARGET_PROPERTIES(luaseri PROPERTIES PREFIX "")

#set(CMAKE_CXX_FLAGS_RELEASE  "-O0")
