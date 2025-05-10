cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug -GNinja -Bbuild
cmake --build build

md .\build\example\shader

glslc -fshader-stage=vertex   shader/2D.vert -o shader/2D_vert.spv
glslc -fshader-stage=fragment shader/2D.frag -o shader/2D_frag.spv
copy .\shader\2D_vert.spv .\build\example\shader\2D_vert.spv
copy .\shader\2D_frag.spv .\build\example\shader\2D_frag.spv

glslc -fshader-stage=compute  shader/SMAA_edge_detection.comp -o shader/SMAA_edge_detection_comp.spv
glslc -fshader-stage=compute  shader/SMAA_blend_weight.comp   -o shader/SMAA_blend_weight_comp.spv
glslc -fshader-stage=compute  shader/SMAA_neighbor.comp       -o shader/SMAA_neighbor_comp.spv
copy .\shader\SMAA_edge_detection_comp.spv .\build\example\shader\SMAA_edge_detection_comp.spv
copy .\shader\SMAA_blend_weight_comp.spv   .\build\example\shader\SMAA_blend_weight_comp.spv
copy .\shader\SMAA_neighbor_comp.spv       .\build\example\shader\SMAA_neighbor_comp.spv