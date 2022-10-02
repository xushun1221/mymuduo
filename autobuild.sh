#!/bin/bash

set -e

# 创建build目录
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

# 清理
rm -rf `pwd`/build/*

# 编译生成
cd `pwd`/build &&
    cmake .. &&
    make

cd ..

# 把头文件拷贝到/usr/include/mymuduo
if [ ! -d /usr/include/mymuduo ]; then
    mkdir /usr/include/mymuduo
fi

for header in `ls *.hh`
do
    cp $header /usr/include/mymuduo
done

# so库拷贝到/usr/lib
cp `pwd`/lib/libmymuduo.so /usr/lib

# 在环境变量中添加了新的so库 刷新一下动态库缓存
ldconfig