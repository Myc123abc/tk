#!/bin/bash

if [ ! -d build ]; then
  cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS="-stdlib=libc++" -DCMAKE_BUILD_TYPE=Debug -GNinja -Bbuild
fi

cmake --build build

mkdir build/example/shader

glslc -fshader-stage=vertex   shader/2D.vert -o shader/2D_vert.spv
glslc -fshader-stage=fragment shader/2D.frag -o shader/2D_frag.spv
cp shader/2D_vert.spv build/example/shader/2D_vert.spv
cp shader/2D_frag.spv build/example/shader/2D_frag.spv

glslc -fshader-stage=vertex   shader/SMAA_edge_detection.vert -o shader/SMAA_edge_detection_vert.spv
glslc -fshader-stage=fragment shader/SMAA_edge_detection.frag -o shader/SMAA_edge_detection_frag.spv
cp shader/SMAA_edge_detection_vert.spv build/example/shader/SMAA_edge_detection_vert.spv
cp shader/SMAA_edge_detection_frag.spv build/example/shader/SMAA_edge_detection_frag.spv

glslc -fshader-stage=vertex   shader/SMAA_blend_weight.vert -o shader/SMAA_blend_weight_vert.spv
glslc -fshader-stage=fragment shader/SMAA_blend_weight.frag -o shader/SMAA_blend_weight_frag.spv
cp shader/SMAA_blend_weight_vert.spv build/example/shader/SMAA_blend_weight_vert.spv
cp shader/SMAA_blend_weight_frag.spv build/example/shader/SMAA_blend_weight_frag.spv