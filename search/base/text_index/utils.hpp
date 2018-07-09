#pragma once

#include "base/checked_cast.hpp"

#include <cstdint>

namespace search
{
namespace base
{
template <typename Sink>
uint32_t RelativePos(Sink & sink, uint64_t startPos)
{
  return ::base::checked_cast<uint32_t>(sink.Pos() - startPos);
}
}  // namespace base
}  // namespace search
