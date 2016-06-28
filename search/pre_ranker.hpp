#pragma once

#include "search/intermediate_result.hpp"

#include "base/macros.hpp"

#include "std/algorithm.hpp"
#include "std/cstdint.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace search
{
// Fast and simple pre-ranker for search results.
class PreRanker
{
public:
  explicit PreRanker(size_t limit);

  template <typename... TArgs>
  void Emplace(TArgs &&... args)
  {
    m_results.emplace_back(forward<TArgs>(args)...);
  }

  void Filter(bool viewportSearch);

  inline size_t Size() const { return m_results.size(); }
  inline size_t Limit() const { return m_limit; }
  inline bool IsEmpty() const { return Size() == 0; }
  inline void Clear() { m_results.clear(); };

  template <typename TFn>
  void ForEach(TFn && fn)
  {
    for_each(m_results.begin(), m_results.end(), forward<TFn>(fn));
  }

  template <typename TFn>
  void ForEachInfo(TFn && fn)
  {
    for (auto & result : m_results)
      fn(result.GetId(), result.GetInfo());
  }

private:
  vector<PreResult1> m_results;
  size_t const m_limit;

  DISALLOW_COPY_AND_MOVE(PreRanker);
};
}  // namespace search
