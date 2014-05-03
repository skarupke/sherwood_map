#!/bin/bash

directory="./static_analysis"

if ! [ -d $directory ]; then
	mkdir $directory
fi
cd $directory
qmake-qt4 ../../sherwood_map.pro -r -spec unsupported/linux-clang CONFIG+=debug CONFIG+=static_analysis
/home/malte/workspace/llvm_trunk/tools/clang/tools/scan-build/scan-build --use-analyzer=/home/malte/workspace/llvm_trunk/build/Release+Asserts/bin/clang++ -v make -j4

