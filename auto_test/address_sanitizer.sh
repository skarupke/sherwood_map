#!/bin/bash

directory="./address_sanitizer"

if ! [ -d $directory ]; then
	mkdir $directory
fi
cd $directory
qmake-qt4 ../../sherwood_map.pro -r -spec unsupported/linux-clang CONFIG+=address_sanitizer
make -j4
ASAN_OPTIONS=check_initialization_order=true:strict_init_order=true ./sherwood_map

