#pragma once

namespace search
{

template <typename T> class MatchCostMock
{
public:
  uint32_t Cost10(T) const { return 1; }
  uint32_t Cost01(T) const { return 1; }
  uint32_t Cost11(T, T) const { return 1; }
  uint32_t Cost12(T a, T const * pB) const
  {
    if (a == 'X' && pB[0] == '>' && pB[1] == '<')
      return 0;
    return 2;
  }
  uint32_t Cost21(T const * pA, T b) const { return Cost12(b, pA); }
  uint32_t Cost22(T const *, T const *) const { return 2; }
  uint32_t SwapCost(T, T) const { return 1; }
};

}  // namespace search
