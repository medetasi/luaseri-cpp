# luaseri-cpp
用 C++序列化 lua 的 table 为 string, 在序列化一些比较大的 table 的时候，用时大概是用 lua 序列化的 1/20
double 转 string 用了 https://github.com/miloyip/dtoa-benchmark/blob/master/src/milo/dtoa_milo.h
效率比标准库的转换快很多
