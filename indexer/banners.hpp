#pragma once

#include <string>

namespace banners
{
struct Banner
{
  enum class Type : uint8_t
  {
    None = 0,
    Facebook = 1
  };

  Banner(Type t, std::string id) : m_type(t), m_bannerId(id) {}

  Type m_type = Type::None;
  std::string m_bannerId;
};
}
