#pragma once

#include "generator/regions/node.hpp"

#include <string>

namespace generator
{
namespace regions
{
class ToStringPolicyInterface
{
public:
  virtual ~ToStringPolicyInterface() = default;

  virtual std::string ToString(NodePath const & path) const = 0;
};

class JsonPolicy : public ToStringPolicyInterface
{
public:
  JsonPolicy(bool extendedOutput = false) : m_extendedOutput(extendedOutput) {}
  JsonPolicy & SetRealPrecision(int32_t precision);
  std::string ToString(NodePath const & path) const override;

public:
  // Longitude and latitude has maximum 3 digits before comma. So we have minimum 6 digits after comma.
  // Nautical mile is good approximation for one angle minute, so we can rely, that
  // final precision is 60 (minutes in degree) * 1852 (meters in one mile) / 1000000 = 0.111 = 111 millimeters.
  // Also, if you are quizzed by nautical mile, just forget, precision was defined in https://jira.mail.ru/browse/MAPSB2B-41
  static uint32_t constexpr kDefaultPrecision = 9;

private:
  bool m_extendedOutput;
  int32_t m_precision = kDefaultPrecision;

};
}  // namespace regions
}  // namespace generator
