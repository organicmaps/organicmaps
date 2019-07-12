#pragma once

#include <string>
#include <utility>
#include <vector>

namespace base
{
namespace url
{
struct Param
{
  Param(std::string const & name, std::string const & value) : m_name(name), m_value(value) {}

  std::string m_name;
  std::string m_value;
};

using Params = std::vector<Param>;

// Make URL by using base url and vector of params.
std::string Make(std::string const & baseUrl, Params const & params);

// Joins URL, appends/removes slashes if needed.
std::string Join(std::string const & lhs, std::string const & rhs);

template <typename... Args>
std::string Join(std::string const & lhs, std::string const & rhs, Args &&... args)
{
  return Join(Join(lhs, rhs), std::forward<Args>(args)...);
}
}  // namespace url
}  // namespace base
