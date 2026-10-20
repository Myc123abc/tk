# Benchmark

A benchmarking utility for performance testing during developement.

## Polyline tessellation

Build and run the standalone CPU benchmark:

```powershell
cmake --build build --target benchmark-polyline
.\build\lib\benchmark\benchmark-polyline.exe
```

`geo` is the current explicit-fringe topology. `tex` uses the same joins with
texture-based fringe coverage and the existing renderer vertex/index formats.
Texture timings cover CPU mesh generation only; atlas management and GPU sampling
are intentionally excluded.
