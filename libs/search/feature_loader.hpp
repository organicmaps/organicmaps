#pragma once

#include "indexer/data_source.hpp"
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
  explicit FeatureLoader(DataSource const & dataSource);

  std::unique_ptr<FeatureType> Load(FeatureID const & id);

  void Reset();

  void ForEachInRect(m2::RectD const & rect, std::function<void(FeatureType &)> const & fn)
  {
    ASSERT(m_checker.CalledOnOriginalThread(), ());
    m_dataSource.ForEachInRect(fn, rect, scales::GetUpperScale());
  }

private:
  DataSource const & m_dataSource;
  std::unique_ptr<FeaturesLoaderGuard> m_guard;

  ThreadChecker m_checker;
};
}  // namespace search
