#pragma once

#define Singleton(ClassName, InstanceName, ...)     \
class ClassName                                     \
{                                                   \
private:                                            \
  ClassName()                            = default; \
  ~ClassName()                           = default; \
public:                                             \
  ClassName(ClassName const&)            = delete;  \
  ClassName(ClassName&&)                 = delete;  \
  ClassName& operator=(ClassName const&) = delete;  \
  ClassName& operator=(ClassName&&)      = delete;  \
                                                    \
  static auto instance() noexcept -> ClassName&     \
  {                                                 \
    static ClassName instance;                      \
    return instance;                                \
  }                                                 \
                                                    \
__VA_ARGS__                                         \
                                                    \
};                                                  \
                                                    \
inline static auto& InstanceName{ ClassName::instance() };

#define Singleton_Derive(ClassName, InstanceName, BaseClass, ...) \
class ClassName : public BaseClass                                \
{                                                                 \
private:                                                          \
  ClassName()                            = default;               \
  ~ClassName()                           = default;               \
public:                                                           \
  ClassName(ClassName const&)            = delete;                \
  ClassName(ClassName&&)                 = delete;                \
  ClassName& operator=(ClassName const&) = delete;                \
  ClassName& operator=(ClassName&&)      = delete;                \
                                                                  \
  static auto instance() noexcept -> ClassName&                   \
  {                                                               \
    static ClassName instance;                                    \
    return instance;                                              \
  }                                                               \
                                                                  \
__VA_ARGS__                                                       \
                                                                  \
};                                                                \
                                                                  \
inline static auto& InstanceName{ ClassName::instance() };