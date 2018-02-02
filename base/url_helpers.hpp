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
}  // namespace url
}  // namespace base
