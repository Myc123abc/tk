cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug -GNinja -Bbuild
cmake --build build

glslc -fshader-stage=fragment shader/2D.frag -o build/2D_frag.spv
glslc -fshader-stage=vertex shader/2D.vert -o build/2D_vert.spv