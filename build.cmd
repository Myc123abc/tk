cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug -GNinja -Bbuild
cmake --build build

md .\build\example\shader

glslc -fshader-stage=vertex   shader/2D.vert -o shader/2D_vert.spv
glslc -fshader-stage=fragment shader/2D.frag -o shader/2D_frag.spv

copy .\shader\2D_vert.spv .\build\example\shader\2D_vert.spv
copy .\shader\2D_frag.spv .\build\example\shader\2D_frag.spv