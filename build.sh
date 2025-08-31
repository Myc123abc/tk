#!/bin/bash

if [ ! -d build ]; then
  cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS="-stdlib=libc++" -DCMAKE_BUILD_TYPE=Debug -GNinja -Bbuild
fi

cmake --build build

mkdir build/example/shader

glslc -fshader-stage=vertex   shader/SDF.vert -o shader/SDF_vert.spv
glslc -fshader-stage=fragment shader/SDF.frag -o shader/SDF_frag.spv
cp shader/SDF_vert.spv build/example/shader/SDF_vert.spv
cp shader/SDF_frag.spv build/example/shader/SDF_frag.spv

mkdir build/example/assets
cp    assets/* build/example/assets/