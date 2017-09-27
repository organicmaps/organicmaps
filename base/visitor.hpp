#pragma once

#define DECLARE_VISITOR(...)          \
  template <typename Visitor>         \
  void Visit(Visitor & visitor)       \
  {                                   \
    __VA_ARGS__;                      \
  }                                   \
  template <typename Visitor>         \
  void Visit(Visitor & visitor) const \
  {                                   \
    __VA_ARGS__;                      \
  }
