#!/bin/bash

if [ ! -d build ]; then
  cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS="-stdlib=libc++" -DCMAKE_BUILD_TYPE=Debug -GNinja -Bbuild
fi

cmake --build build

mkdir build/example/shader

glslc -fshader-stage=fragment shader/2D.frag -o build/example/shader/2D_frag.spv
glslc -fshader-stage=vertex   shader/2D.vert -o build/example/shader/2D_vert.spv