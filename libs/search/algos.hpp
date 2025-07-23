#pragma once

#include "base/base.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace search
{
namespace impl
{
struct LS
{
  size_t prevDecreasePos, decreaseValue;
  size_t prevIncreasePos, increaseValue;

  LS(size_t i)
  {
    prevDecreasePos = i;
    decreaseValue = 1;
    prevIncreasePos = i;
    increaseValue = 1;
  }
};
}  // namespace impl

template <typename T, typename OutIter, typename Comp>
void LongestSubsequence(std::vector<T> const & in, OutIter out, Comp cmp)
{
  if (in.empty())
    return;

  std::vector<impl::LS> v;
  v.reserve(in.size());
  for (size_t i = 0; i < in.size(); ++i)
    v.push_back(impl::LS(i));

  size_t res = 1;
  size_t pos = 0;
  for (size_t i = 0; i < v.size(); ++i)
  {
    for (size_t j = i + 1; j < v.size(); ++j)
    {
      if (cmp.Less(in[i], in[j]) && v[i].increaseValue + 1 >= v[j].increaseValue)
      {
        v[j].increaseValue = v[i].increaseValue + 1;
        v[j].prevIncreasePos = i;
      }

      if (cmp.Greater(in[i], in[j]) && v[i].decreaseValue + 1 >= v[j].decreaseValue)
      {
        v[j].decreaseValue = v[i].decreaseValue + 1;
        v[j].prevDecreasePos = i;
      }

      size_t const m = std::max(v[j].increaseValue, v[j].decreaseValue);
      if (m > res)
      {
        res = m;
        pos = j;
      }
    }
  }

  bool increasing = true;
  if (v[pos].increaseValue < v[pos].decreaseValue)
    increasing = false;

  while (res-- > 0)
  {
    *out++ = in[pos];

    if (increasing)
      pos = v[pos].prevIncreasePos;
    else
      pos = v[pos].prevDecreasePos;
  }
}
}  // namespace search
