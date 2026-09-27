# tk-util

A header-only C++ utility library.

## Utilities
|Utility|Description|
|-|-|
|base|Common utility types, such as `uint`, `uint16`, `float2`, etc.|
|error_handling|Simple error handling utilities.|
|flag|Flag type supporting boolean operators and helper functions.|
|log|Simple logging library supporting different log levels with color output.|
|rect|Rectangle type designed as a replacement for Win32 `RECT`.|
|variant|Wrapper around `std::variant` with simplified `visit` support and helper types for pattern matching.|
|tuple|Wrapper around `std::tuple` with simplified `apply` support.|
|free_list|Free list used to track free pointers.|
|memory_pool|Memory pool that specifies the sizes of inner fixed-pools.|
