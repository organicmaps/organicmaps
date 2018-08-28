#pragma once

#include "metrics/eye_info.hpp"

#include "base/exception.hpp"

#include <cstdint>
#include <vector>

namespace eye
{
class Serdes
{
public:
  DECLARE_EXCEPTION(UnknownVersion, RootException);

  static void Serialize(Info const & info, std::vector<int8_t> & result);
  static void Deserialize(std::vector<int8_t> const & bytes, Info & result);
};
}  // namespace eye
