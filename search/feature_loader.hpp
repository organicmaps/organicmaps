#pragma once

#include "indexer/index.hpp"
#include "indexer/scales.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"
#include "base/thread_checker.hpp"

#include <memory>
#include <utility>

class FeatureType;
struct FeatureID;

namespace search
{
class FeatureLoader
{
public:
  explicit FeatureLoader(Index const & index);

  WARN_UNUSED_RESULT bool Load(FeatureID const & id, FeatureType & ft);

  void Reset();

  template <typename ToDo>
  void ForEachInRect(m2::RectD const & rect, ToDo && toDo)
  {
    ASSERT(m_checker.CalledOnOriginalThread(), ());
    m_index.ForEachInRect(std::forward<ToDo>(toDo), rect, scales::GetUpperScale());
  }

private:
  Index const & m_index;
  std::unique_ptr<Index::FeaturesLoaderGuard> m_guard;

  ThreadChecker m_checker;
};
}  // namespace search
