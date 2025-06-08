#pragma once

#if defined(STATIC_TK)
  #define TK_API
#else
  #if defined(_WIN32)
    #if defined(SHARED_TK)
      #define TK_API __declspec(dllexport)
    #else
      #define TK_API __declspec(dllimport)
    #endif
  #else
    #error "not implement import export interface of other platform"
  #endif
#endif