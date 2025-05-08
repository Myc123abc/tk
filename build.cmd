cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug -GNinja -Bbuild
cmake --build build

md .\build\example\shader

glslc -fshader-stage=vertex   shader/2D.vert -o shader/2D_vert.spv
glslc -fshader-stage=fragment shader/2D.frag -o shader/2D_frag.spv
copy .\shader\2D_vert.spv .\build\example\shader\2D_vert.spv
copy .\shader\2D_frag.spv .\build\example\shader\2D_frag.spv

glslc -fshader-stage=vertex   shader/SMAA_edge_detection.vert -o shader/SMAA_edge_detection_vert.spv
glslc -fshader-stage=fragment shader/SMAA_edge_detection.frag -o shader/SMAA_edge_detection_frag.spv
copy .\shader\SMAA_edge_detection_vert.spv .\build\example\shader\SMAA_edge_detection_vert.spv
copy .\shader\SMAA_edge_detection_frag.spv .\build\example\shader\SMAA_edge_detection_frag.spv

glslc -fshader-stage=vertex   shader/SMAA_blend_weight.vert -o shader/SMAA_blend_weight_vert.spv
glslc -fshader-stage=fragment shader/SMAA_blend_weight.frag -o shader/SMAA_blend_weight_frag.spv
copy .\shader\SMAA_blend_weight_vert.spv .\build\example\shader\SMAA_blend_weight_vert.spv
copy .\shader\SMAA_blend_weight_frag.spv .\build\example\shader\SMAA_blend_weight_frag.spv

glslc -fshader-stage=vertex   shader/SMAA_neighbor.vert -o shader/SMAA_neighbor_vert.spv
glslc -fshader-stage=fragment shader/SMAA_neighbor.frag -o shader/SMAA_neighbor_frag.spv
copy .\shader\SMAA_neighbor_vert.spv .\build\example\shader\SMAA_neighbor_vert.spv
copy .\shader\SMAA_neighbor_frag.spv .\build\example\shader\SMAA_neighbor_frag.spv