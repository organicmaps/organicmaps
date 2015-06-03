#pragma once

namespace search
{

/// Upper bound for max count of tokens for indexing and scoring.
int constexpr MAX_TOKENS = 32;
int constexpr MAX_SUGGESTS_COUNT = 5;

template <typename IterT1, typename IterT2>
bool StartsWith(IterT1 beg, IterT1 end, IterT2 begPrefix, IterT2 endPrefix)
{
  while (beg != end && begPrefix != endPrefix && *beg == *begPrefix)
  {
    ++beg;
    ++begPrefix;
  }
  return begPrefix == endPrefix;
}

}  // namespace search
