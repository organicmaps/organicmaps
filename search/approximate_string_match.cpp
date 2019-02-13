#include "search/approximate_string_match.hpp"

// TODO: Сделать модель ошибок.
// Учитывать соседние кнопки на клавиатуре.
// 1. Сосед вместо нужной
// 2. Сосед до или после нужной.

namespace search
{
using strings::UniChar;

uint32_t DefaultMatchCost::Cost10(UniChar) const
{
  return 256;
}

uint32_t DefaultMatchCost::Cost01(UniChar) const
{
  return 256;
}

uint32_t DefaultMatchCost::Cost11(UniChar, UniChar) const
{
  return 256;
}

uint32_t DefaultMatchCost::Cost12(UniChar, UniChar const *) const
{
  return 512;
}

uint32_t DefaultMatchCost::Cost21(UniChar const *, UniChar) const
{
  return 512;
}

uint32_t DefaultMatchCost::Cost22(UniChar const *, UniChar const *) const
{
  return 512;
}

uint32_t DefaultMatchCost::SwapCost(UniChar, UniChar) const
{
  return 256;
}
}  // namespace search
