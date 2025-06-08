cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -GNinja -Bbuild .
cmake --build build

md .\build\example\shader

glslc -fshader-stage=vertex   shader/SDF.vert -o shader/SDF_vert.spv
glslc -fshader-stage=fragment shader/SDF.frag -o shader/SDF_frag.spv
copy .\shader\SDF_vert.spv .\build\example\shader\SDF_vert.spv
copy .\shader\SDF_frag.spv .\build\example\shader\SDF_frag.spv

md   .\build\example\resources
copy .\resources\* .\build\example\resources\

glslc -fshader-stage=vertex   shader/text_render.vert -o shader/text_render_vert.spv
glslc -fshader-stage=fragment shader/text_render.frag -o shader/text_render_frag.spv
copy .\shader\text_render_vert.spv .\build\example\shader\text_render_vert.spv
copy .\shader\text_render_frag.spv .\build\example\shader\text_render_frag.spv

copy .\build\tk.dll .\build\example\
copy .\build\tk.pdb .\build\example\