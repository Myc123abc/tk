cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug -GNinja -Bbuild
cmake --build build

md .\build\example\shader

glslc -fshader-stage=vertex   shader/SDF.vert -o shader/SDF_vert.spv
glslc -fshader-stage=fragment shader/SDF.frag -o shader/SDF_frag.spv
copy .\shader\SDF_vert.spv .\build\example\shader\SDF_vert.spv
copy .\shader\SDF_frag.spv .\build\example\shader\SDF_frag.spv

md   .\build\example\resources
copy .\resources\* .\build\resources\