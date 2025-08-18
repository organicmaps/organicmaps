#pragma once

#include <memory>
#include <type_traits>

namespace generator
{
template <typename T, typename... Args>
std::enable_if_t<std::is_constructible<T, Args...>::value, std::shared_ptr<T>> create(Args &&... args)
{
  return std::make_shared<T>(std::forward<Args>(args)...);
}

// impossible to construct
template <typename T, typename... Args>
std::enable_if_t<!std::is_constructible<T, Args...>::value, std::shared_ptr<T>> create(Args &&...)
{
  return nullptr;
}
}  // namespace generator
